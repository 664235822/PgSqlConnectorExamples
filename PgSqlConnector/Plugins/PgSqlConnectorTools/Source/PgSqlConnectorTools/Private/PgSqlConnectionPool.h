// Copyright HaoJunDeveloper 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/Event.h"
#include "PgSqlTypes.h"

class FPgSqlLibpqApi;
struct pg_conn;
using PGconn = pg_conn;

class FPgSqlPooledConnection : public TSharedFromThis<FPgSqlPooledConnection, ESPMode::ThreadSafe>
{
public:
	FPgSqlPooledConnection(FPgSqlLibpqApi& InApi, const FPgSqlConnectionConfig& InConfig, int32 InConnectionId);
	~FPgSqlPooledConnection();

	bool Open(FString& OutError);
	void Close();
	bool IsHealthy() const;

	FPgSqlQueryResult ExecuteQuery(const FString& Sql);
	FPgSqlTransactionResult ExecuteTransaction(const TArray<FPgSqlStatement>& Statements);

	int32 GetConnectionId() const { return ConnectionId; }

private:
	FPgSqlQueryResult ExecuteStatementInternal(const FString& Sql);

	FPgSqlLibpqApi& Api;
	FPgSqlConnectionConfig Config;
	int32 ConnectionId = INDEX_NONE;
	PGconn* Connection = nullptr;
	mutable FCriticalSection ConnectionMutex;
};

class FPgSqlConnectionPool : public TSharedFromThis<FPgSqlConnectionPool, ESPMode::ThreadSafe>
{
public:
	FPgSqlConnectionPool(FPgSqlLibpqApi& InApi, FPgSqlConnectionConfig InConfig);
	~FPgSqlConnectionPool();

	TSharedPtr<FPgSqlPooledConnection, ESPMode::ThreadSafe> Acquire(FString& OutError);
	void Release(const TSharedPtr<FPgSqlPooledConnection, ESPMode::ThreadSafe>& Connection, bool bHealthy);
	void Shutdown();

	const FPgSqlConnectionConfig& GetConfig() const { return Config; }

private:
	void WakeOneWaiter_NoLock();

	FPgSqlLibpqApi& Api;
	FPgSqlConnectionConfig Config;
	FCriticalSection Mutex;
	TArray<TSharedPtr<FPgSqlPooledConnection, ESPMode::ThreadSafe>> AllConnections;
	TArray<TSharedPtr<FPgSqlPooledConnection, ESPMode::ThreadSafe>> IdleConnections;
	TArray<FEvent*> Waiters;
	int32 ReservedConnectionSlots = 0;
	int32 NextConnectionId = 1;
	bool bShuttingDown = false;
};
