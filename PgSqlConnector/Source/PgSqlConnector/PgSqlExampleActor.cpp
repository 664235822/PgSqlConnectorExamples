#include "PgSqlExampleActor.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "PgSqlConnectionHandle.h"
#include "PgSqlConnectorSubsystem.h"
#include "PgSqlTypes.h"

APgSqlExampleActor::APgSqlExampleActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APgSqlExampleActor::BeginPlay()
{
	Super::BeginPlay();

	PrintStatus(TEXT("PgSqlExampleActor BeginPlay triggered."), FColor::Silver);

	if (bRunDemoOnBeginPlay)
	{
		RunHelloDemo();
	}
	else
	{
		PrintStatus(TEXT("PgSql demo is disabled. Enable bRunDemoOnBeginPlay or call RunHelloDemo manually."), FColor::Orange);
	}
}

bool APgSqlExampleActor::ConnectToDatabase(FString& OutError)
{
	UPgSqlConnectionHandle* ConnectionHandle = GetOrCreateConnection(OutError);
	const bool bConnected = ConnectionHandle != nullptr;

	if (bConnected)
	{
		PrintStatus(TEXT("PostgreSQL connected successfully."), FColor::Green);
	}
	else
	{
		PrintStatus(FString::Printf(TEXT("PostgreSQL connect failed: %s"), *OutError), FColor::Red);
	}

	return bConnected;
}

void APgSqlExampleActor::DisconnectFromDatabase()
{
	if (Connection != nullptr)
	{
		Connection->Disconnect();
		PrintStatus(TEXT("PostgreSQL disconnected."), FColor::Yellow);
	}
}

void APgSqlExampleActor::RunHelloDemo()
{
	FString Error;
	UPgSqlConnectionHandle* ConnectionHandle = GetOrCreateConnection(Error);
	if (ConnectionHandle == nullptr)
	{
		PrintStatus(FString::Printf(TEXT("Connect failed: %s"), *Error), FColor::Red);
		return;
	}

	PrintStatus(TEXT("Connected to PostgreSQL."), FColor::Green);

	const FPgSqlQueryResult ColumnResult = ConnectionHandle->ExecuteQuery(TEXT("SELECT * FROM hello LIMIT 0;"));
	if (!ColumnResult.bSucceeded)
	{
		PrintStatus(FString::Printf(TEXT("Query column names failed: %s"), *ColumnResult.ErrorMessage), FColor::Red);
		return;
	}

	const FString ColumnNames = ColumnResult.ColumnNames.Num() > 0
		? FString::Join(ColumnResult.ColumnNames, TEXT(", "))
		: TEXT("(no columns)");
	PrintStatus(FString::Printf(TEXT("hello columns: %s"), *ColumnNames), FColor::Cyan);

	const FString InsertSql = FString::Printf(
		TEXT("INSERT INTO hello (name) VALUES ('%s');"),
		*EscapeSqlLiteral(DemoInsertName));

	const FPgSqlQueryResult InsertResult = ConnectionHandle->ExecuteQuery(InsertSql);
	if (!InsertResult.bSucceeded)
	{
		PrintStatus(FString::Printf(TEXT("Insert failed: %s"), *InsertResult.ErrorMessage), FColor::Red);
		return;
	}

	PrintStatus(FString::Printf(TEXT("Inserted name into hello: %s"), *DemoInsertName), FColor::Green);

	const FPgSqlQueryResult DataResult = ConnectionHandle->ExecuteQuery(TEXT("SELECT * FROM hello;"));
	if (!DataResult.bSucceeded)
	{
		PrintStatus(FString::Printf(TEXT("Query data failed: %s"), *DataResult.ErrorMessage), FColor::Red);
		return;
	}

	PrintStatus(FString::Printf(TEXT("hello rows: %s"), *FormatQueryResultRows(DataResult)), FColor::Yellow);
}

FString APgSqlExampleActor::QueryHelloTable()
{
	FString Error;
	UPgSqlConnectionHandle* ConnectionHandle = GetOrCreateConnection(Error);
	if (ConnectionHandle == nullptr)
	{
		return Error;
	}

	const FPgSqlQueryResult Result = ConnectionHandle->ExecuteQuery(TEXT("SELECT * FROM hello;"));
	if (!Result.bSucceeded)
	{
		return Result.ErrorMessage;
	}

	return FormatQueryResultRows(Result);
}

bool APgSqlExampleActor::InsertHelloName(const FString& InName, FString& OutError)
{
	UPgSqlConnectionHandle* ConnectionHandle = GetOrCreateConnection(OutError);
	if (ConnectionHandle == nullptr)
	{
		PrintStatus(FString::Printf(TEXT("Insert failed before execution: %s"), *OutError), FColor::Red);
		return false;
	}

	const FString Sql = FString::Printf(
		TEXT("INSERT INTO hello (name) VALUES ('%s');"),
		*EscapeSqlLiteral(InName));

	const FPgSqlQueryResult Result = ConnectionHandle->ExecuteQuery(Sql);
	if (!Result.bSucceeded)
	{
		OutError = Result.ErrorMessage;
		PrintStatus(FString::Printf(TEXT("Insert failed: %s"), *OutError), FColor::Red);
		return false;
	}

	OutError.Reset();
	PrintStatus(FString::Printf(TEXT("Inserted hello.name = %s"), *InName), FColor::Green);
	return true;
}

void APgSqlExampleActor::PrintStatus(const FString& Message, const FColor& Color) const
{
	UE_LOG(LogTemp, Log, TEXT("%s"), *Message);

	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			8.0f,
			Color,
			Message);
	}
}

FString APgSqlExampleActor::EscapeSqlLiteral(const FString& Value) const
{
	return Value.Replace(TEXT("'"), TEXT("''"));
}

FString APgSqlExampleActor::FormatQueryResultRows(const FPgSqlQueryResult& Result) const
{
	if (Result.Rows.Num() == 0)
	{
		return TEXT("(no rows)");
	}

	TArray<FString> RowStrings;
	RowStrings.Reserve(Result.Rows.Num());

	for (const FPgSqlResultRow& Row : Result.Rows)
	{
		TArray<FString> ColumnStrings;
		ColumnStrings.Reserve(Row.Columns.Num());

		for (const FPgSqlColumnValue& Column : Row.Columns)
		{
			ColumnStrings.Add(FString::Printf(
				TEXT("%s=%s"),
				*Column.Name,
				Column.bIsNull ? TEXT("NULL") : *Column.Value));
		}

		RowStrings.Add(FString::Printf(TEXT("{ %s }"), *FString::Join(ColumnStrings, TEXT(", "))));
	}

	return FString::Join(RowStrings, TEXT(" | "));
}

UPgSqlConnectionHandle* APgSqlExampleActor::GetOrCreateConnection(FString& OutError)
{
	if (Connection != nullptr && Connection->IsConnected())
	{
		OutError.Reset();
		return Connection;
	}

	UWorld* World = GetWorld();
	UGameInstance* GameInstance = World != nullptr ? World->GetGameInstance() : nullptr;
	UPgSqlConnectorSubsystem* Subsystem = GameInstance != nullptr ? GameInstance->GetSubsystem<UPgSqlConnectorSubsystem>() : nullptr;
	if (Subsystem == nullptr)
	{
		OutError = TEXT("PgSqlConnectorSubsystem is unavailable.");
		return nullptr;
	}

	if (Connection == nullptr)
	{
		FPgSqlConnectionConfig Config;
		Config.Host = Host;
		Config.Port = Port;
		Config.Database = Database;
		Config.User = User;
		Config.Password = Password;
		Config.PoolName = PoolName;
		Connection = Subsystem->CreateConnectionForOuter(this, Config);
	}

	if (!Connection->IsConnected() && !Connection->Connect(OutError))
	{
		return nullptr;
	}

	OutError.Reset();
	return Connection;
}
