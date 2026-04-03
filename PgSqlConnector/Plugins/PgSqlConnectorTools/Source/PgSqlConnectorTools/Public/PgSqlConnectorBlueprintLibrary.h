// Copyright HaoJunDeveloper 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PgSqlTypes.h"
#include "PgSqlConnectorBlueprintLibrary.generated.h"

class UPgSqlConnectionHandle;
class UPgSqlConnectorSubsystem;

UCLASS()
class PGSQLCONNECTORTOOLS_API UPgSqlConnectorBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "PgSqlConnector|Subsystem", meta = (WorldContext = "WorldContextObject"))
	static UPgSqlConnectorSubsystem* GetPgSqlConnectorSubsystem(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "PgSqlConnector|Connection", meta = (WorldContext = "WorldContextObject"))
	static UPgSqlConnectionHandle* CreatePgSqlConnection(const UObject* WorldContextObject, const FPgSqlConnectionConfig& Config);

	UFUNCTION(BlueprintPure, Category = "PgSqlConnector|Config", meta = (
		AdvancedDisplay = "PoolName,ConnectTimeoutSeconds,AcquireTimeoutSeconds,MaxPoolSize,ClientEncoding,ApplicationName,AdditionalOptions",
		CPP_Default_Host = "127.0.0.1",
		CPP_Default_Port = "5432",
		CPP_Default_Database = "",
		CPP_Default_User = "",
		CPP_Default_Password = "",
		CPP_Default_PoolName = "",
		CPP_Default_ConnectTimeoutSeconds = "5",
		CPP_Default_AcquireTimeoutSeconds = "5",
		CPP_Default_MaxPoolSize = "8",
		CPP_Default_ClientEncoding = "UTF8",
		CPP_Default_ApplicationName = "PgSqlConnectorTools",
		CPP_Default_AdditionalOptions = ""
	))
	static FPgSqlConnectionConfig MakeConnectionConfig(
		const FString& Host,
		int32 Port,
		const FString& Database,
		const FString& User,
		const FString& Password,
		const FString& PoolName,
		int32 ConnectTimeoutSeconds,
		int32 AcquireTimeoutSeconds,
		int32 MaxPoolSize,
		const FString& ClientEncoding,
		const FString& ApplicationName,
		const FString& AdditionalOptions);

	UFUNCTION(BlueprintPure, Category = "PgSqlConnector|Transaction")
	static FPgSqlStatement MakeStatement(const FString& Sql);

	UFUNCTION(BlueprintPure, Category = "PgSqlConnector|Result")
	static bool GetRowValue(const FPgSqlResultRow& Row, const FString& ColumnName, FString& OutValue);

	UFUNCTION(BlueprintPure, Category = "PgSqlConnector|Result")
	static bool GetCellValue(const FPgSqlQueryResult& Result, int32 RowIndex, const FString& ColumnName, FString& OutValue);
};
