// Copyright HaoJunDeveloper 2026. All Rights Reserved.

#pragma once

#include "Async/Future.h"
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PgSqlTypes.h"
#include "PgSqlConnectionHandle.generated.h"

class UPgSqlConnectorSubsystem;

UCLASS(BlueprintType)
class PGSQLCONNECTORTOOLS_API UPgSqlConnectionHandle : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UPgSqlConnectorSubsystem* InSubsystem, const FPgSqlConnectionConfig& InConfig);

	virtual void BeginDestroy() override;

	UFUNCTION(BlueprintCallable, Category = "PgSqlConnector|Connection")
	bool Connect(FString& OutError);

	UFUNCTION(BlueprintCallable, Category = "PgSqlConnector|Connection")
	void Disconnect();

	UFUNCTION(BlueprintPure, Category = "PgSqlConnector|Connection")
	bool IsConnected() const;

	UFUNCTION(BlueprintPure, Category = "PgSqlConnector|Connection")
	FString GetConnectionKey() const { return ConnectionKey; }

	UFUNCTION(BlueprintPure, Category = "PgSqlConnector|Connection")
	FPgSqlConnectionConfig GetConfig() const { return Config; }

	UFUNCTION(BlueprintCallable, Category = "PgSqlConnector|Query")
	FPgSqlQueryResult ExecuteQuery(const FString& Sql);

	UFUNCTION(BlueprintCallable, Category = "PgSqlConnector|Transaction", meta = (AutoCreateRefTerm = "Statements"))
	FPgSqlTransactionResult ExecuteTransaction(const TArray<FPgSqlStatement>& Statements);

	TFuture<FPgSqlQueryResult> ExecuteQueryFuture(const FString& Sql);
	TFuture<FPgSqlTransactionResult> ExecuteTransactionFuture(const TArray<FPgSqlStatement>& Statements);

	void ExecuteQueryAsync(const FString& Sql, FPgSqlQueryNativeDelegate Callback);
	void ExecuteTransactionAsync(const TArray<FPgSqlStatement>& Statements, FPgSqlTransactionNativeDelegate Callback);

private:
	UPROPERTY(Transient)
	TObjectPtr<UPgSqlConnectorSubsystem> ConnectorSubsystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PgSqlConnector|Connection", meta = (AllowPrivateAccess = "true"))
	FPgSqlConnectionConfig Config;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "PgSqlConnector|Connection", meta = (AllowPrivateAccess = "true"))
	FString ConnectionKey;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "PgSqlConnector|Connection", meta = (AllowPrivateAccess = "true"))
	bool bConnected = false;
};
