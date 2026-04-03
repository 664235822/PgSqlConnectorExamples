// Copyright HaoJunDeveloper 2026. All Rights Reserved.

#include "PgSqlConnectorBlueprintLibrary.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "PgSqlConnectionHandle.h"
#include "PgSqlConnectorSubsystem.h"

UPgSqlConnectorSubsystem* UPgSqlConnectorBlueprintLibrary::GetPgSqlConnectorSubsystem(const UObject* WorldContextObject)
{
	if (WorldContextObject == nullptr)
	{
		return nullptr;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	UGameInstance* GameInstance = World != nullptr ? World->GetGameInstance() : nullptr;
	return GameInstance != nullptr ? GameInstance->GetSubsystem<UPgSqlConnectorSubsystem>() : nullptr;
}

UPgSqlConnectionHandle* UPgSqlConnectorBlueprintLibrary::CreatePgSqlConnection(const UObject* WorldContextObject, const FPgSqlConnectionConfig& Config)
{
	if (UPgSqlConnectorSubsystem* Subsystem = GetPgSqlConnectorSubsystem(WorldContextObject))
	{
		return Subsystem->CreateConnection(Config);
	}

	return nullptr;
}

FPgSqlConnectionConfig UPgSqlConnectorBlueprintLibrary::MakeConnectionConfig(
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
	const FString& AdditionalOptions)
{
	FPgSqlConnectionConfig Config;
	Config.Host = Host;
	Config.Port = Port;
	Config.Database = Database;
	Config.User = User;
	Config.Password = Password;
	Config.PoolName = PoolName;
	Config.ConnectTimeoutSeconds = ConnectTimeoutSeconds;
	Config.AcquireTimeoutSeconds = AcquireTimeoutSeconds;
	Config.MaxPoolSize = MaxPoolSize;
	Config.ClientEncoding = ClientEncoding;
	Config.ApplicationName = ApplicationName;
	Config.AdditionalOptions = AdditionalOptions;
	return Config;
}

FPgSqlStatement UPgSqlConnectorBlueprintLibrary::MakeStatement(const FString& Sql)
{
	FPgSqlStatement Statement;
	Statement.Sql = Sql;
	return Statement;
}

bool UPgSqlConnectorBlueprintLibrary::GetRowValue(const FPgSqlResultRow& Row, const FString& ColumnName, FString& OutValue)
{
	if (const FString* FoundValue = Row.FindValue(ColumnName))
	{
		OutValue = *FoundValue;
		return true;
	}

	OutValue.Reset();
	return false;
}

bool UPgSqlConnectorBlueprintLibrary::GetCellValue(const FPgSqlQueryResult& Result, int32 RowIndex, const FString& ColumnName, FString& OutValue)
{
	if (!Result.Rows.IsValidIndex(RowIndex))
	{
		OutValue.Reset();
		return false;
	}

	return GetRowValue(Result.Rows[RowIndex], ColumnName, OutValue);
}
