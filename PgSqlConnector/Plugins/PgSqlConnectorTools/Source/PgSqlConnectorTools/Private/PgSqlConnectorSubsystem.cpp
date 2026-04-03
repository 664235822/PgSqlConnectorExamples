// Copyright HaoJunDeveloper 2026. All Rights Reserved.

#include "PgSqlConnectorSubsystem.h"

#include "Async/Async.h"
#include "PgSqlConnectionHandle.h"
#include "PgSqlConnectionPool.h"
#include "PgSqlConnectorTools.h"
#include "PgSqlLibpqApi.h"

void UPgSqlConnectorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FString Diagnostic;
	if (FPgSqlLibpqApi::Get().EnsureLoaded(Diagnostic))
	{
		UE_LOG(LogPgSqlConnectorTools, Log, TEXT("libpq runtime loaded: %s"), *FPgSqlLibpqApi::Get().GetResolvedDllPath());
	}
	else
	{
		UE_LOG(LogPgSqlConnectorTools, Warning, TEXT("libpq runtime not available yet: %s"), *Diagnostic);
	}
}

void UPgSqlConnectorSubsystem::Deinitialize()
{
	TArray<TSharedPtr<FPgSqlConnectionPool, ESPMode::ThreadSafe>> PoolsToShutdown;
	{
		FScopeLock Lock(&PoolsMutex);
		Pools.GenerateValueArray(PoolsToShutdown);
		Pools.Reset();
		PoolReferenceCounts.Reset();
	}

	for (const TSharedPtr<FPgSqlConnectionPool, ESPMode::ThreadSafe>& Pool : PoolsToShutdown)
	{
		if (Pool.IsValid())
		{
			Pool->Shutdown();
		}
	}

	Super::Deinitialize();
}

bool UPgSqlConnectorSubsystem::IsConnectorReady(FString& OutDiagnostic) const
{
	if (!FPgSqlLibpqApi::Get().EnsureLoaded(OutDiagnostic))
	{
		return false;
	}

	OutDiagnostic = FString::Printf(TEXT("libpq loaded: %s"), *FPgSqlLibpqApi::Get().GetResolvedDllPath());
	return true;
}

UPgSqlConnectionHandle* UPgSqlConnectorSubsystem::CreateConnection(const FPgSqlConnectionConfig& Config)
{
	return CreateConnectionForOuter(this, Config);
}

UPgSqlConnectionHandle* UPgSqlConnectorSubsystem::CreateConnectionForOuter(UObject* Outer, const FPgSqlConnectionConfig& Config)
{
	UObject* ObjectOuter = Outer != nullptr ? Outer : this;
	UPgSqlConnectionHandle* Connection = NewObject<UPgSqlConnectionHandle>(ObjectOuter);
	Connection->Initialize(this, Config);
	return Connection;
}

bool UPgSqlConnectorSubsystem::OpenConnection(const FPgSqlConnectionConfig& Config, FString& OutConnectionKey, FString& OutError)
{
	FString PoolError;
	const TSharedPtr<FPgSqlConnectionPool, ESPMode::ThreadSafe> Pool = GetOrCreatePool(Config, PoolError);
	if (!Pool.IsValid())
	{
		OutError = PoolError;
		return false;
	}

	FString AcquireError;
	const TSharedPtr<FPgSqlPooledConnection, ESPMode::ThreadSafe> Connection = Pool->Acquire(AcquireError);
	if (!Connection.IsValid())
	{
		OutError = AcquireError;
		return false;
	}

	Pool->Release(Connection, true);

	const FString PoolKey = Config.ToPoolKey();
	{
		FScopeLock Lock(&PoolsMutex);
		PoolReferenceCounts.FindOrAdd(PoolKey) += 1;
	}

	OutConnectionKey = PoolKey;
	OutError.Reset();
	return true;
}

void UPgSqlConnectorSubsystem::CloseConnection(const FString& ConnectionKey)
{
	TSharedPtr<FPgSqlConnectionPool, ESPMode::ThreadSafe> PoolToShutdown;

	{
		FScopeLock Lock(&PoolsMutex);
		int32* ReferenceCount = PoolReferenceCounts.Find(ConnectionKey);
		if (ReferenceCount == nullptr)
		{
			return;
		}

		*ReferenceCount -= 1;
		if (*ReferenceCount > 0)
		{
			return;
		}

		PoolReferenceCounts.Remove(ConnectionKey);
		if (TSharedPtr<FPgSqlConnectionPool, ESPMode::ThreadSafe>* ExistingPool = Pools.Find(ConnectionKey))
		{
			PoolToShutdown = *ExistingPool;
			Pools.Remove(ConnectionKey);
		}
	}

	if (PoolToShutdown.IsValid())
	{
		PoolToShutdown->Shutdown();
	}
}

bool UPgSqlConnectorSubsystem::IsConnectionOpen(const FString& ConnectionKey) const
{
	FScopeLock Lock(&PoolsMutex);
	const int32* ReferenceCount = PoolReferenceCounts.Find(ConnectionKey);
	return ReferenceCount != nullptr && *ReferenceCount > 0 && Pools.Contains(ConnectionKey);
}

FPgSqlQueryResult UPgSqlConnectorSubsystem::ExecuteQuery(const FPgSqlConnectionConfig& Config, const FString& Sql)
{
	if (Sql.TrimStartAndEnd().IsEmpty())
	{
		return FPgSqlQueryResult::MakeError(TEXT("SQL cannot be empty."));
	}

	FString PoolError;
	const TSharedPtr<FPgSqlConnectionPool, ESPMode::ThreadSafe> Pool = GetOrCreatePool(Config, PoolError);
	if (!Pool.IsValid())
	{
		return FPgSqlQueryResult::MakeError(PoolError);
	}

	FString AcquireError;
	const TSharedPtr<FPgSqlPooledConnection, ESPMode::ThreadSafe> Connection = Pool->Acquire(AcquireError);
	if (!Connection.IsValid())
	{
		return FPgSqlQueryResult::MakeError(AcquireError);
	}

	const FPgSqlQueryResult Result = Connection->ExecuteQuery(Sql);
	Pool->Release(Connection, Result.bConnectionHealthy);
	return Result;
}

FPgSqlTransactionResult UPgSqlConnectorSubsystem::ExecuteTransaction(const FPgSqlConnectionConfig& Config, const TArray<FPgSqlStatement>& Statements)
{
	if (Statements.IsEmpty())
	{
		return FPgSqlTransactionResult::MakeError(TEXT("Transaction statements cannot be empty."));
	}

	FString PoolError;
	const TSharedPtr<FPgSqlConnectionPool, ESPMode::ThreadSafe> Pool = GetOrCreatePool(Config, PoolError);
	if (!Pool.IsValid())
	{
		return FPgSqlTransactionResult::MakeError(PoolError);
	}

	FString AcquireError;
	const TSharedPtr<FPgSqlPooledConnection, ESPMode::ThreadSafe> Connection = Pool->Acquire(AcquireError);
	if (!Connection.IsValid())
	{
		return FPgSqlTransactionResult::MakeError(AcquireError);
	}

	const FPgSqlTransactionResult Result = Connection->ExecuteTransaction(Statements);
	Pool->Release(Connection, Result.bConnectionHealthy);
	return Result;
}

FPgSqlQueryResult UPgSqlConnectorSubsystem::ExecuteConnectedQuery(const FString& ConnectionKey, const FString& Sql)
{
	if (Sql.TrimStartAndEnd().IsEmpty())
	{
		return FPgSqlQueryResult::MakeError(TEXT("SQL cannot be empty."));
	}

	FString PoolError;
	const TSharedPtr<FPgSqlConnectionPool, ESPMode::ThreadSafe> Pool = GetExistingPool(ConnectionKey, PoolError);
	if (!Pool.IsValid())
	{
		return FPgSqlQueryResult::MakeError(PoolError);
	}

	FString AcquireError;
	const TSharedPtr<FPgSqlPooledConnection, ESPMode::ThreadSafe> Connection = Pool->Acquire(AcquireError);
	if (!Connection.IsValid())
	{
		return FPgSqlQueryResult::MakeError(AcquireError);
	}

	const FPgSqlQueryResult Result = Connection->ExecuteQuery(Sql);
	Pool->Release(Connection, Result.bConnectionHealthy);
	return Result;
}

FPgSqlTransactionResult UPgSqlConnectorSubsystem::ExecuteConnectedTransaction(const FString& ConnectionKey, const TArray<FPgSqlStatement>& Statements)
{
	if (Statements.IsEmpty())
	{
		return FPgSqlTransactionResult::MakeError(TEXT("Transaction statements cannot be empty."));
	}

	FString PoolError;
	const TSharedPtr<FPgSqlConnectionPool, ESPMode::ThreadSafe> Pool = GetExistingPool(ConnectionKey, PoolError);
	if (!Pool.IsValid())
	{
		return FPgSqlTransactionResult::MakeError(PoolError);
	}

	FString AcquireError;
	const TSharedPtr<FPgSqlPooledConnection, ESPMode::ThreadSafe> Connection = Pool->Acquire(AcquireError);
	if (!Connection.IsValid())
	{
		return FPgSqlTransactionResult::MakeError(AcquireError);
	}

	const FPgSqlTransactionResult Result = Connection->ExecuteTransaction(Statements);
	Pool->Release(Connection, Result.bConnectionHealthy);
	return Result;
}

TFuture<FPgSqlQueryResult> UPgSqlConnectorSubsystem::ExecuteQueryFuture(const FPgSqlConnectionConfig& Config, const FString& Sql)
{
	TPromise<FPgSqlQueryResult> Promise;
	TFuture<FPgSqlQueryResult> Future = Promise.GetFuture();
	TWeakObjectPtr<UPgSqlConnectorSubsystem> WeakThis(this);

	Async(EAsyncExecution::ThreadPool, [WeakThis, Config, Sql, Promise = MoveTemp(Promise)]() mutable
	{
		if (UPgSqlConnectorSubsystem* Subsystem = WeakThis.Get())
		{
			Promise.SetValue(Subsystem->ExecuteQuery(Config, Sql));
			return;
		}

		Promise.SetValue(FPgSqlQueryResult::MakeError(TEXT("PgSqlConnectorSubsystem is no longer valid.")));
	});

	return Future;
}

TFuture<FPgSqlTransactionResult> UPgSqlConnectorSubsystem::ExecuteTransactionFuture(const FPgSqlConnectionConfig& Config, const TArray<FPgSqlStatement>& Statements)
{
	TPromise<FPgSqlTransactionResult> Promise;
	TFuture<FPgSqlTransactionResult> Future = Promise.GetFuture();
	TWeakObjectPtr<UPgSqlConnectorSubsystem> WeakThis(this);

	Async(EAsyncExecution::ThreadPool, [WeakThis, Config, Statements, Promise = MoveTemp(Promise)]() mutable
	{
		if (UPgSqlConnectorSubsystem* Subsystem = WeakThis.Get())
		{
			Promise.SetValue(Subsystem->ExecuteTransaction(Config, Statements));
			return;
		}

		Promise.SetValue(FPgSqlTransactionResult::MakeError(TEXT("PgSqlConnectorSubsystem is no longer valid.")));
	});

	return Future;
}

TFuture<FPgSqlQueryResult> UPgSqlConnectorSubsystem::ExecuteConnectedQueryFuture(const FString& ConnectionKey, const FString& Sql)
{
	TPromise<FPgSqlQueryResult> Promise;
	TFuture<FPgSqlQueryResult> Future = Promise.GetFuture();
	TWeakObjectPtr<UPgSqlConnectorSubsystem> WeakThis(this);

	Async(EAsyncExecution::ThreadPool, [WeakThis, ConnectionKey, Sql, Promise = MoveTemp(Promise)]() mutable
	{
		if (UPgSqlConnectorSubsystem* Subsystem = WeakThis.Get())
		{
			Promise.SetValue(Subsystem->ExecuteConnectedQuery(ConnectionKey, Sql));
			return;
		}

		Promise.SetValue(FPgSqlQueryResult::MakeError(TEXT("PgSqlConnectorSubsystem is no longer valid.")));
	});

	return Future;
}

TFuture<FPgSqlTransactionResult> UPgSqlConnectorSubsystem::ExecuteConnectedTransactionFuture(const FString& ConnectionKey, const TArray<FPgSqlStatement>& Statements)
{
	TPromise<FPgSqlTransactionResult> Promise;
	TFuture<FPgSqlTransactionResult> Future = Promise.GetFuture();
	TWeakObjectPtr<UPgSqlConnectorSubsystem> WeakThis(this);

	Async(EAsyncExecution::ThreadPool, [WeakThis, ConnectionKey, Statements, Promise = MoveTemp(Promise)]() mutable
	{
		if (UPgSqlConnectorSubsystem* Subsystem = WeakThis.Get())
		{
			Promise.SetValue(Subsystem->ExecuteConnectedTransaction(ConnectionKey, Statements));
			return;
		}

		Promise.SetValue(FPgSqlTransactionResult::MakeError(TEXT("PgSqlConnectorSubsystem is no longer valid.")));
	});

	return Future;
}

void UPgSqlConnectorSubsystem::ExecuteQueryAsync(const FPgSqlConnectionConfig& Config, const FString& Sql, FPgSqlQueryNativeDelegate Callback)
{
	TWeakObjectPtr<UPgSqlConnectorSubsystem> WeakThis(this);

	Async(EAsyncExecution::ThreadPool, [WeakThis, Config, Sql, Callback = MoveTemp(Callback)]() mutable
	{
		FPgSqlQueryResult Result = FPgSqlQueryResult::MakeError(TEXT("PgSqlConnectorSubsystem is no longer valid."));
		if (UPgSqlConnectorSubsystem* Subsystem = WeakThis.Get())
		{
			Result = Subsystem->ExecuteQuery(Config, Sql);
		}

		AsyncTask(ENamedThreads::GameThread, [Callback = MoveTemp(Callback), Result]() mutable
		{
			if (Callback.IsBound())
			{
				Callback.Execute(Result);
			}
		});
	});
}

void UPgSqlConnectorSubsystem::ExecuteTransactionAsync(const FPgSqlConnectionConfig& Config, const TArray<FPgSqlStatement>& Statements, FPgSqlTransactionNativeDelegate Callback)
{
	TWeakObjectPtr<UPgSqlConnectorSubsystem> WeakThis(this);

	Async(EAsyncExecution::ThreadPool, [WeakThis, Config, Statements, Callback = MoveTemp(Callback)]() mutable
	{
		FPgSqlTransactionResult Result = FPgSqlTransactionResult::MakeError(TEXT("PgSqlConnectorSubsystem is no longer valid."));
		if (UPgSqlConnectorSubsystem* Subsystem = WeakThis.Get())
		{
			Result = Subsystem->ExecuteTransaction(Config, Statements);
		}

		AsyncTask(ENamedThreads::GameThread, [Callback = MoveTemp(Callback), Result]() mutable
		{
			if (Callback.IsBound())
			{
				Callback.Execute(Result);
			}
		});
	});
}

void UPgSqlConnectorSubsystem::ExecuteConnectedQueryAsync(const FString& ConnectionKey, const FString& Sql, FPgSqlQueryNativeDelegate Callback)
{
	TWeakObjectPtr<UPgSqlConnectorSubsystem> WeakThis(this);

	Async(EAsyncExecution::ThreadPool, [WeakThis, ConnectionKey, Sql, Callback = MoveTemp(Callback)]() mutable
	{
		FPgSqlQueryResult Result = FPgSqlQueryResult::MakeError(TEXT("PgSqlConnectorSubsystem is no longer valid."));
		if (UPgSqlConnectorSubsystem* Subsystem = WeakThis.Get())
		{
			Result = Subsystem->ExecuteConnectedQuery(ConnectionKey, Sql);
		}

		AsyncTask(ENamedThreads::GameThread, [Callback = MoveTemp(Callback), Result]() mutable
		{
			if (Callback.IsBound())
			{
				Callback.Execute(Result);
			}
		});
	});
}

void UPgSqlConnectorSubsystem::ExecuteConnectedTransactionAsync(const FString& ConnectionKey, const TArray<FPgSqlStatement>& Statements, FPgSqlTransactionNativeDelegate Callback)
{
	TWeakObjectPtr<UPgSqlConnectorSubsystem> WeakThis(this);

	Async(EAsyncExecution::ThreadPool, [WeakThis, ConnectionKey, Statements, Callback = MoveTemp(Callback)]() mutable
	{
		FPgSqlTransactionResult Result = FPgSqlTransactionResult::MakeError(TEXT("PgSqlConnectorSubsystem is no longer valid."));
		if (UPgSqlConnectorSubsystem* Subsystem = WeakThis.Get())
		{
			Result = Subsystem->ExecuteConnectedTransaction(ConnectionKey, Statements);
		}

		AsyncTask(ENamedThreads::GameThread, [Callback = MoveTemp(Callback), Result]() mutable
		{
			if (Callback.IsBound())
			{
				Callback.Execute(Result);
			}
		});
	});
}

TSharedPtr<FPgSqlConnectionPool, ESPMode::ThreadSafe> UPgSqlConnectorSubsystem::GetOrCreatePool(const FPgSqlConnectionConfig& Config, FString& OutError)
{
	if (!Config.IsValidConfig(OutError))
	{
		return nullptr;
	}

	if (!FPgSqlLibpqApi::Get().EnsureLoaded(OutError))
	{
		return nullptr;
	}

	const FString PoolKey = Config.ToPoolKey();

	FScopeLock Lock(&PoolsMutex);
	if (const TSharedPtr<FPgSqlConnectionPool, ESPMode::ThreadSafe>* ExistingPool = Pools.Find(PoolKey))
	{
		return *ExistingPool;
	}

	TSharedPtr<FPgSqlConnectionPool, ESPMode::ThreadSafe> NewPool = MakeShared<FPgSqlConnectionPool, ESPMode::ThreadSafe>(FPgSqlLibpqApi::Get(), Config);
	Pools.Add(PoolKey, NewPool);
	return NewPool;
}

TSharedPtr<FPgSqlConnectionPool, ESPMode::ThreadSafe> UPgSqlConnectorSubsystem::GetExistingPool(const FString& ConnectionKey, FString& OutError) const
{
	FScopeLock Lock(&PoolsMutex);

	const int32* ReferenceCount = PoolReferenceCounts.Find(ConnectionKey);
	if (ReferenceCount == nullptr || *ReferenceCount <= 0)
	{
		OutError = TEXT("The connection is not open.");
		return nullptr;
	}

	if (const TSharedPtr<FPgSqlConnectionPool, ESPMode::ThreadSafe>* ExistingPool = Pools.Find(ConnectionKey))
	{
		return *ExistingPool;
	}

	OutError = TEXT("The connection pool no longer exists.");
	return nullptr;
}
