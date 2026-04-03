// Copyright HaoJunDeveloper 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PgSqlTypes.generated.h"

DECLARE_DELEGATE_OneParam(FPgSqlQueryNativeDelegate, const struct FPgSqlQueryResult&);
DECLARE_DELEGATE_OneParam(FPgSqlTransactionNativeDelegate, const struct FPgSqlTransactionResult&);

UENUM(BlueprintType)
enum class EPgSqlQueryStatus : uint8
{
	Unknown UMETA(DisplayName = "Unknown"),
	EmptyQuery UMETA(DisplayName = "Empty Query"),
	CommandOk UMETA(DisplayName = "Command OK"),
	TuplesOk UMETA(DisplayName = "Tuples OK"),
	BadResponse UMETA(DisplayName = "Bad Response"),
	NonFatalError UMETA(DisplayName = "Non Fatal Error"),
	FatalError UMETA(DisplayName = "Fatal Error"),
	SingleTuple UMETA(DisplayName = "Single Tuple")
};

USTRUCT(BlueprintType)
struct PGSQLCONNECTORTOOLS_API FPgSqlColumnValue
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "PgSqlConnector")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "PgSqlConnector")
	FString Value;

	UPROPERTY(BlueprintReadOnly, Category = "PgSqlConnector")
	bool bIsNull = false;
};

USTRUCT(BlueprintType)
struct PGSQLCONNECTORTOOLS_API FPgSqlResultRow
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "PgSqlConnector")
	TArray<FPgSqlColumnValue> Columns;

	const FString* FindValue(const FString& ColumnName) const
	{
		for (const FPgSqlColumnValue& Column : Columns)
		{
			if (Column.Name.Equals(ColumnName, ESearchCase::IgnoreCase))
			{
				return Column.bIsNull ? nullptr : &Column.Value;
			}
		}

		return nullptr;
	}
};

USTRUCT(BlueprintType)
struct PGSQLCONNECTORTOOLS_API FPgSqlQueryResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "PgSqlConnector")
	bool bSucceeded = false;

	UPROPERTY(BlueprintReadOnly, Category = "PgSqlConnector")
	bool bConnectionHealthy = true;

	UPROPERTY(BlueprintReadOnly, Category = "PgSqlConnector")
	EPgSqlQueryStatus Status = EPgSqlQueryStatus::Unknown;

	UPROPERTY(BlueprintReadOnly, Category = "PgSqlConnector")
	FString ErrorMessage;

	UPROPERTY(BlueprintReadOnly, Category = "PgSqlConnector")
	FString CommandStatus;

	UPROPERTY(BlueprintReadOnly, Category = "PgSqlConnector")
	int32 AffectedRows = 0;

	UPROPERTY(BlueprintReadOnly, Category = "PgSqlConnector")
	int32 ConnectionId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "PgSqlConnector")
	double DurationMilliseconds = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "PgSqlConnector")
	TArray<FString> ColumnNames;

	UPROPERTY(BlueprintReadOnly, Category = "PgSqlConnector")
	TArray<FPgSqlResultRow> Rows;

	static FPgSqlQueryResult MakeError(const FString& InError, int32 InConnectionId = INDEX_NONE)
	{
		FPgSqlQueryResult Result;
		Result.bSucceeded = false;
		Result.bConnectionHealthy = false;
		Result.Status = EPgSqlQueryStatus::FatalError;
		Result.ErrorMessage = InError;
		Result.ConnectionId = InConnectionId;
		return Result;
	}
};

USTRUCT(BlueprintType)
struct PGSQLCONNECTORTOOLS_API FPgSqlStatement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PgSqlConnector")
	FString Sql;
};

USTRUCT(BlueprintType)
struct PGSQLCONNECTORTOOLS_API FPgSqlTransactionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "PgSqlConnector")
	bool bSucceeded = false;

	UPROPERTY(BlueprintReadOnly, Category = "PgSqlConnector")
	bool bConnectionHealthy = true;

	UPROPERTY(BlueprintReadOnly, Category = "PgSqlConnector")
	FString ErrorMessage;

	UPROPERTY(BlueprintReadOnly, Category = "PgSqlConnector")
	int32 ConnectionId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "PgSqlConnector")
	double DurationMilliseconds = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "PgSqlConnector")
	TArray<FPgSqlQueryResult> StatementResults;

	static FPgSqlTransactionResult MakeError(const FString& InError, int32 InConnectionId = INDEX_NONE)
	{
		FPgSqlTransactionResult Result;
		Result.bSucceeded = false;
		Result.bConnectionHealthy = false;
		Result.ErrorMessage = InError;
		Result.ConnectionId = InConnectionId;
		return Result;
	}
};

USTRUCT(BlueprintType)
struct PGSQLCONNECTORTOOLS_API FPgSqlConnectionConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PgSqlConnector")
	FString PoolName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PgSqlConnector")
	FString Host = TEXT("127.0.0.1");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PgSqlConnector")
	int32 Port = 5432;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PgSqlConnector")
	FString Database;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PgSqlConnector")
	FString User;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PgSqlConnector")
	FString Password;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PgSqlConnector")
	int32 MaxPoolSize = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PgSqlConnector")
	int32 ConnectTimeoutSeconds = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PgSqlConnector")
	int32 AcquireTimeoutSeconds = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PgSqlConnector")
	FString ClientEncoding = TEXT("UTF8");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PgSqlConnector")
	FString ApplicationName = TEXT("PgSqlConnectorTools");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PgSqlConnector")
	FString AdditionalOptions;

	bool IsValidConfig(FString& OutError) const
	{
		if (Host.IsEmpty())
		{
			OutError = TEXT("Host cannot be empty.");
			return false;
		}

		if (Database.IsEmpty())
		{
			OutError = TEXT("Database cannot be empty.");
			return false;
		}

		if (User.IsEmpty())
		{
			OutError = TEXT("User cannot be empty.");
			return false;
		}

		if (Port <= 0)
		{
			OutError = TEXT("Port must be greater than zero.");
			return false;
		}

		if (MaxPoolSize <= 0)
		{
			OutError = TEXT("MaxPoolSize must be greater than zero.");
			return false;
		}

		if (AcquireTimeoutSeconds < 0 || ConnectTimeoutSeconds < 0)
		{
			OutError = TEXT("Timeout values cannot be negative.");
			return false;
		}

		OutError.Reset();
		return true;
	}

	FString ToPoolKey() const
	{
		if (!PoolName.IsEmpty())
		{
			return PoolName;
		}

		return FString::Printf(TEXT("%s:%d/%s?user=%s&pool=%d&extra=%s"),
			*Host,
			Port,
			*Database,
			*User,
			MaxPoolSize,
			*AdditionalOptions);
	}

	FString ToConnectionString() const
	{
		TArray<FString> Parts;
		Parts.Reserve(8);

		Parts.Add(FString::Printf(TEXT("host='%s'"), *EscapeConnectionValue(Host)));
		Parts.Add(FString::Printf(TEXT("port='%d'"), Port));
		Parts.Add(FString::Printf(TEXT("dbname='%s'"), *EscapeConnectionValue(Database)));
		Parts.Add(FString::Printf(TEXT("user='%s'"), *EscapeConnectionValue(User)));

		if (!Password.IsEmpty())
		{
			Parts.Add(FString::Printf(TEXT("password='%s'"), *EscapeConnectionValue(Password)));
		}

		if (ConnectTimeoutSeconds > 0)
		{
			Parts.Add(FString::Printf(TEXT("connect_timeout='%d'"), ConnectTimeoutSeconds));
		}

		if (!ClientEncoding.IsEmpty())
		{
			Parts.Add(FString::Printf(TEXT("client_encoding='%s'"), *EscapeConnectionValue(ClientEncoding)));
		}

		if (!ApplicationName.IsEmpty())
		{
			Parts.Add(FString::Printf(TEXT("application_name='%s'"), *EscapeConnectionValue(ApplicationName)));
		}

		if (!AdditionalOptions.IsEmpty())
		{
			Parts.Add(AdditionalOptions);
		}

		return FString::Join(Parts, TEXT(" "));
	}

private:
	static FString EscapeConnectionValue(const FString& Value)
	{
		FString Escaped = Value.Replace(TEXT("\\"), TEXT("\\\\"));
		Escaped = Escaped.Replace(TEXT("'"), TEXT("\\'"));
		return Escaped;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPgSqlQueryResultDynamicDelegate, const FPgSqlQueryResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPgSqlTransactionResultDynamicDelegate, const FPgSqlTransactionResult&, Result);
