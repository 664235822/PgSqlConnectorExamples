// Copyright HaoJunDeveloper 2026. All Rights Reserved.

#pragma once

#include "Async/Future.h"
#include "CoreMinimal.h"
#include "PgSqlTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PgSqlConnectorSubsystem.generated.h"

class FPgSqlConnectionPool;
class UPgSqlConnectionHandle;

UCLASS(BlueprintType)
class PGSQLCONNECTORTOOLS_API UPgSqlConnectorSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "PgSqlConnector")
	bool IsConnectorReady(FString& OutDiagnostic) const;

	UFUNCTION(BlueprintCallable, Category = "PgSqlConnector|Connection")
	UPgSqlConnectionHandle* CreateConnection(const FPgSqlConnectionConfig& Config);

	UPgSqlConnectionHandle* CreateConnectionForOuter(UObject* Outer, const FPgSqlConnectionConfig& Config);

	bool OpenConnection(const FPgSqlConnectionConfig& Config, FString& OutConnectionKey, FString& OutError);
	void CloseConnection(const FString& ConnectionKey);
	bool IsConnectionOpen(const FString& ConnectionKey) const;

	FPgSqlQueryResult ExecuteQuery(const FPgSqlConnectionConfig& Config, const FString& Sql);
	FPgSqlTransactionResult ExecuteTransaction(const FPgSqlConnectionConfig& Config, const TArray<FPgSqlStatement>& Statements);
	FPgSqlQueryResult ExecuteConnectedQuery(const FString& ConnectionKey, const FString& Sql);
	FPgSqlTransactionResult ExecuteConnectedTransaction(const FString& ConnectionKey, const TArray<FPgSqlStatement>& Statements);

	TFuture<FPgSqlQueryResult> ExecuteQueryFuture(const FPgSqlConnectionConfig& Config, const FString& Sql);
	TFuture<FPgSqlTransactionResult> ExecuteTransactionFuture(const FPgSqlConnectionConfig& Config, const TArray<FPgSqlStatement>& Statements);
	TFuture<FPgSqlQueryResult> ExecuteConnectedQueryFuture(const FString& ConnectionKey, const FString& Sql);
	TFuture<FPgSqlTransactionResult> ExecuteConnectedTransactionFuture(const FString& ConnectionKey, const TArray<FPgSqlStatement>& Statements);

	void ExecuteQueryAsync(const FPgSqlConnectionConfig& Config, const FString& Sql, FPgSqlQueryNativeDelegate Callback);
	void ExecuteTransactionAsync(const FPgSqlConnectionConfig& Config, const TArray<FPgSqlStatement>& Statements, FPgSqlTransactionNativeDelegate Callback);
	void ExecuteConnectedQueryAsync(const FString& ConnectionKey, const FString& Sql, FPgSqlQueryNativeDelegate Callback);
	void ExecuteConnectedTransactionAsync(const FString& ConnectionKey, const TArray<FPgSqlStatement>& Statements, FPgSqlTransactionNativeDelegate Callback);

private:
	TSharedPtr<FPgSqlConnectionPool, ESPMode::ThreadSafe> GetOrCreatePool(const FPgSqlConnectionConfig& Config, FString& OutError);
	TSharedPtr<FPgSqlConnectionPool, ESPMode::ThreadSafe> GetExistingPool(const FString& ConnectionKey, FString& OutError) const;

	mutable FCriticalSection PoolsMutex;
	TMap<FString, TSharedPtr<FPgSqlConnectionPool, ESPMode::ThreadSafe>> Pools;
	TMap<FString, int32> PoolReferenceCounts;
};
