// Copyright HaoJunDeveloper 2026. All Rights Reserved.

#include "PgSqlConnectionPool.h"

#include "PgSqlConnectorTools.h"
#include "PgSqlLibpqApi.h"

#include "HAL/PlatformProcess.h"
#include "Misc/ScopeExit.h"

namespace PgSqlConnectorConstants
{
	constexpr int32 CONNECTION_OK = 0;
	constexpr int32 PGRES_EMPTY_QUERY = 0;
	constexpr int32 PGRES_COMMAND_OK = 1;
	constexpr int32 PGRES_TUPLES_OK = 2;
	constexpr int32 PGRES_BAD_RESPONSE = 5;
	constexpr int32 PGRES_NONFATAL_ERROR = 6;
	constexpr int32 PGRES_FATAL_ERROR = 7;
	constexpr int32 PGRES_SINGLE_TUPLE = 9;

	EPgSqlQueryStatus ToQueryStatus(int32 NativeStatus)
	{
		switch (NativeStatus)
		{
		case PGRES_EMPTY_QUERY:
			return EPgSqlQueryStatus::EmptyQuery;
		case PGRES_COMMAND_OK:
			return EPgSqlQueryStatus::CommandOk;
		case PGRES_TUPLES_OK:
			return EPgSqlQueryStatus::TuplesOk;
		case PGRES_BAD_RESPONSE:
			return EPgSqlQueryStatus::BadResponse;
		case PGRES_NONFATAL_ERROR:
			return EPgSqlQueryStatus::NonFatalError;
		case PGRES_FATAL_ERROR:
			return EPgSqlQueryStatus::FatalError;
		case PGRES_SINGLE_TUPLE:
			return EPgSqlQueryStatus::SingleTuple;
		default:
			return EPgSqlQueryStatus::Unknown;
		}
	}

	FString Utf8ToString(const char* Value)
	{
		return Value != nullptr ? UTF8_TO_TCHAR(Value) : FString();
	}
}

FPgSqlPooledConnection::FPgSqlPooledConnection(FPgSqlLibpqApi& InApi, const FPgSqlConnectionConfig& InConfig, int32 InConnectionId)
	: Api(InApi)
	, Config(InConfig)
	, ConnectionId(InConnectionId)
{
}

FPgSqlPooledConnection::~FPgSqlPooledConnection()
{
	Close();
}

bool FPgSqlPooledConnection::Open(FString& OutError)
{
	FScopeLock Lock(&ConnectionMutex);

	if (Connection != nullptr)
	{
		OutError.Reset();
		return true;
	}

	const FString ConnectionString = Config.ToConnectionString();
	const FTCHARToUTF8 ConnectionStringUtf8(*ConnectionString);

	Connection = Api.ConnectDb(ConnectionStringUtf8.Get());
	if (Connection == nullptr)
	{
		OutError = TEXT("PQconnectdb returned a null connection handle.");
		return false;
	}

	if (Api.Status(Connection) != PgSqlConnectorConstants::CONNECTION_OK)
	{
		OutError = PgSqlConnectorConstants::Utf8ToString(Api.ErrorMessage(Connection));
		Api.Finish(Connection);
		Connection = nullptr;
		return false;
	}

	if (!Config.ClientEncoding.IsEmpty())
	{
		const FTCHARToUTF8 EncodingUtf8(*Config.ClientEncoding);
		Api.SetClientEncoding(Connection, EncodingUtf8.Get());
	}

	UE_LOG(LogPgSqlConnectorTools, Log, TEXT("Opened PostgreSQL connection #%d to %s:%d/%s"), ConnectionId, *Config.Host, Config.Port, *Config.Database);
	OutError.Reset();
	return true;
}

void FPgSqlPooledConnection::Close()
{
	FScopeLock Lock(&ConnectionMutex);

	if (Connection != nullptr)
	{
		Api.Finish(Connection);
		Connection = nullptr;
	}
}

bool FPgSqlPooledConnection::IsHealthy() const
{
	FScopeLock Lock(&ConnectionMutex);
	return Connection != nullptr && Api.Status(Connection) == PgSqlConnectorConstants::CONNECTION_OK;
}

FPgSqlQueryResult FPgSqlPooledConnection::ExecuteQuery(const FString& Sql)
{
	return ExecuteStatementInternal(Sql);
}

FPgSqlTransactionResult FPgSqlPooledConnection::ExecuteTransaction(const TArray<FPgSqlStatement>& Statements)
{
	const double StartSeconds = FPlatformTime::Seconds();

	if (Statements.IsEmpty())
	{
		return FPgSqlTransactionResult::MakeError(TEXT("Transaction statements cannot be empty."), ConnectionId);
	}

	FPgSqlTransactionResult TransactionResult;
	TransactionResult.ConnectionId = ConnectionId;

	FPgSqlQueryResult BeginResult = ExecuteStatementInternal(TEXT("BEGIN"));
	if (!BeginResult.bSucceeded)
	{
		TransactionResult.ErrorMessage = BeginResult.ErrorMessage;
		TransactionResult.bConnectionHealthy = BeginResult.bConnectionHealthy;
		TransactionResult.DurationMilliseconds = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;
		return TransactionResult;
	}

	for (const FPgSqlStatement& Statement : Statements)
	{
		FPgSqlQueryResult StatementResult = ExecuteStatementInternal(Statement.Sql);
		TransactionResult.StatementResults.Add(StatementResult);

		if (!StatementResult.bSucceeded)
		{
			FPgSqlQueryResult RollbackResult = ExecuteStatementInternal(TEXT("ROLLBACK"));
			TransactionResult.bSucceeded = false;
			TransactionResult.bConnectionHealthy = StatementResult.bConnectionHealthy && RollbackResult.bConnectionHealthy;
			TransactionResult.ErrorMessage = StatementResult.ErrorMessage;
			TransactionResult.DurationMilliseconds = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;
			return TransactionResult;
		}
	}

	FPgSqlQueryResult CommitResult = ExecuteStatementInternal(TEXT("COMMIT"));
	TransactionResult.bSucceeded = CommitResult.bSucceeded;
	TransactionResult.bConnectionHealthy = CommitResult.bConnectionHealthy;
	TransactionResult.ErrorMessage = CommitResult.ErrorMessage;
	TransactionResult.DurationMilliseconds = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;
	return TransactionResult;
}

FPgSqlQueryResult FPgSqlPooledConnection::ExecuteStatementInternal(const FString& Sql)
{
	const double StartSeconds = FPlatformTime::Seconds();

	if (Sql.TrimStartAndEnd().IsEmpty())
	{
		return FPgSqlQueryResult::MakeError(TEXT("SQL cannot be empty."), ConnectionId);
	}

	FScopeLock Lock(&ConnectionMutex);

	if (Connection == nullptr)
	{
		return FPgSqlQueryResult::MakeError(TEXT("The connection has not been opened yet."), ConnectionId);
	}

	FPgSqlQueryResult Result;
	Result.ConnectionId = ConnectionId;

	const FTCHARToUTF8 SqlUtf8(*Sql);
	PGresult* NativeResult = Api.Exec(Connection, SqlUtf8.Get());

	if (NativeResult == nullptr)
	{
		Result = FPgSqlQueryResult::MakeError(PgSqlConnectorConstants::Utf8ToString(Api.ErrorMessage(Connection)), ConnectionId);
		Result.DurationMilliseconds = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;
		Result.bConnectionHealthy = Api.Status(Connection) == PgSqlConnectorConstants::CONNECTION_OK;
		return Result;
	}

	ON_SCOPE_EXIT
	{
		Api.Clear(NativeResult);
	};

	const int32 NativeStatus = Api.ResultStatus(NativeResult);
	Result.Status = PgSqlConnectorConstants::ToQueryStatus(NativeStatus);
	Result.CommandStatus = PgSqlConnectorConstants::Utf8ToString(Api.CmdStatus(NativeResult));
	Result.bConnectionHealthy = Api.Status(Connection) == PgSqlConnectorConstants::CONNECTION_OK;

	if (const char* AffectedRows = Api.CmdTuples(NativeResult); AffectedRows != nullptr && AffectedRows[0] != '\0')
	{
		Result.AffectedRows = FCString::Atoi(UTF8_TO_TCHAR(AffectedRows));
	}

	const bool bSuccessStatus =
		NativeStatus == PgSqlConnectorConstants::PGRES_COMMAND_OK ||
		NativeStatus == PgSqlConnectorConstants::PGRES_TUPLES_OK ||
		NativeStatus == PgSqlConnectorConstants::PGRES_SINGLE_TUPLE;

	Result.bSucceeded = bSuccessStatus;
	if (!bSuccessStatus)
	{
		Result.ErrorMessage = PgSqlConnectorConstants::Utf8ToString(Api.ErrorMessage(Connection));
		Result.DurationMilliseconds = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;
		return Result;
	}

	const int32 FieldCount = Api.NumFields(NativeResult);
	Result.ColumnNames.Reserve(FieldCount);
	for (int32 FieldIndex = 0; FieldIndex < FieldCount; ++FieldIndex)
	{
		Result.ColumnNames.Add(PgSqlConnectorConstants::Utf8ToString(Api.FieldName(NativeResult, FieldIndex)));
	}

	const int32 RowCount = Api.NumTuples(NativeResult);
	Result.Rows.Reserve(RowCount);

	for (int32 RowIndex = 0; RowIndex < RowCount; ++RowIndex)
	{
		FPgSqlResultRow Row;
		Row.Columns.Reserve(FieldCount);

		for (int32 FieldIndex = 0; FieldIndex < FieldCount; ++FieldIndex)
		{
			FPgSqlColumnValue Column;
			Column.Name = Result.ColumnNames.IsValidIndex(FieldIndex) ? Result.ColumnNames[FieldIndex] : FString::Printf(TEXT("Column_%d"), FieldIndex);
			Column.bIsNull = Api.GetIsNull(NativeResult, RowIndex, FieldIndex) != 0;
			Column.Value = Column.bIsNull ? FString() : PgSqlConnectorConstants::Utf8ToString(Api.GetValue(NativeResult, RowIndex, FieldIndex));
			Row.Columns.Add(MoveTemp(Column));
		}

		Result.Rows.Add(MoveTemp(Row));
	}

	Result.DurationMilliseconds = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;
	return Result;
}

FPgSqlConnectionPool::FPgSqlConnectionPool(FPgSqlLibpqApi& InApi, FPgSqlConnectionConfig InConfig)
	: Api(InApi)
	, Config(MoveTemp(InConfig))
{
}

FPgSqlConnectionPool::~FPgSqlConnectionPool()
{
	Shutdown();
}

TSharedPtr<FPgSqlPooledConnection, ESPMode::ThreadSafe> FPgSqlConnectionPool::Acquire(FString& OutError)
{
	const double Deadline = FPlatformTime::Seconds() + FMath::Max(0, Config.AcquireTimeoutSeconds);

	for (;;)
	{
		bool bShouldCreateConnection = false;
		int32 NewConnectionId = INDEX_NONE;

		{
			FScopeLock Lock(&Mutex);

			if (bShuttingDown)
			{
				OutError = TEXT("The connection pool is shutting down.");
				return nullptr;
			}

			while (!IdleConnections.IsEmpty())
			{
				TSharedPtr<FPgSqlPooledConnection, ESPMode::ThreadSafe> ExistingConnection = IdleConnections.Pop(EAllowShrinking::No);
				if (ExistingConnection.IsValid() && ExistingConnection->IsHealthy())
				{
					OutError.Reset();
					return ExistingConnection;
				}

				AllConnections.RemoveSingleSwap(ExistingConnection, EAllowShrinking::No);
				ReservedConnectionSlots = FMath::Max(0, ReservedConnectionSlots - 1);
			}

			if (ReservedConnectionSlots < Config.MaxPoolSize)
			{
				++ReservedConnectionSlots;
				NewConnectionId = NextConnectionId++;
				bShouldCreateConnection = true;
			}
		}

		if (bShouldCreateConnection)
		{
			TSharedPtr<FPgSqlPooledConnection, ESPMode::ThreadSafe> NewConnection = MakeShared<FPgSqlPooledConnection, ESPMode::ThreadSafe>(Api, Config, NewConnectionId);
			if (!NewConnection->Open(OutError))
			{
				FScopeLock Lock(&Mutex);
				ReservedConnectionSlots = FMath::Max(0, ReservedConnectionSlots - 1);
				return nullptr;
			}

			FScopeLock Lock(&Mutex);
			AllConnections.Add(NewConnection);
			return NewConnection;
		}

		const double RemainingSeconds = Deadline - FPlatformTime::Seconds();
		if (RemainingSeconds <= 0.0)
		{
			OutError = FString::Printf(TEXT("Timed out waiting for a pooled connection (Pool=%s, MaxPoolSize=%d)."), *Config.ToPoolKey(), Config.MaxPoolSize);
			return nullptr;
		}

		FEvent* Waiter = FPlatformProcess::GetSynchEventFromPool(true);
		{
			FScopeLock Lock(&Mutex);
			Waiters.Add(Waiter);
		}

		Waiter->Wait(static_cast<uint32>(RemainingSeconds * 1000.0));

		{
			FScopeLock Lock(&Mutex);
			Waiters.RemoveSingleSwap(Waiter, EAllowShrinking::No);
		}

		FPlatformProcess::ReturnSynchEventToPool(Waiter);
	}
}

void FPgSqlConnectionPool::Release(const TSharedPtr<FPgSqlPooledConnection, ESPMode::ThreadSafe>& Connection, bool bHealthy)
{
	if (!Connection.IsValid())
	{
		return;
	}

	FScopeLock Lock(&Mutex);

	if (bShuttingDown || !bHealthy)
	{
		AllConnections.RemoveSingleSwap(Connection, EAllowShrinking::No);
		ReservedConnectionSlots = FMath::Max(0, ReservedConnectionSlots - 1);
		Connection->Close();
		return;
	}

	IdleConnections.Add(Connection);
	WakeOneWaiter_NoLock();
}

void FPgSqlConnectionPool::Shutdown()
{
	FScopeLock Lock(&Mutex);

	if (bShuttingDown)
	{
		return;
	}

	bShuttingDown = true;
	for (FEvent* Waiter : Waiters)
	{
		if (Waiter != nullptr)
		{
			Waiter->Trigger();
		}
	}

	for (const TSharedPtr<FPgSqlPooledConnection, ESPMode::ThreadSafe>& Connection : AllConnections)
	{
		if (Connection.IsValid())
		{
			Connection->Close();
		}
	}

	AllConnections.Reset();
	IdleConnections.Reset();
	ReservedConnectionSlots = 0;
}

void FPgSqlConnectionPool::WakeOneWaiter_NoLock()
{
	if (!Waiters.IsEmpty())
	{
		if (FEvent* Waiter = Waiters.Last())
		{
			Waiter->Trigger();
		}
	}
}
