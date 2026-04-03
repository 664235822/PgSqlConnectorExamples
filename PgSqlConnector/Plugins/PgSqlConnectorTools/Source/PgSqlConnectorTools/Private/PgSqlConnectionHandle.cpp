// Copyright HaoJunDeveloper 2026. All Rights Reserved.

#include "PgSqlConnectionHandle.h"

#include "PgSqlConnectorSubsystem.h"

void UPgSqlConnectionHandle::Initialize(UPgSqlConnectorSubsystem* InSubsystem, const FPgSqlConnectionConfig& InConfig)
{
	if (!bConnected)
	{
		Config = InConfig;
	}

	ConnectorSubsystem = InSubsystem;
}

void UPgSqlConnectionHandle::BeginDestroy()
{
	Disconnect();
	Super::BeginDestroy();
}

bool UPgSqlConnectionHandle::Connect(FString& OutError)
{
	if (bConnected)
	{
		OutError.Reset();
		return true;
	}

	if (ConnectorSubsystem == nullptr)
	{
		OutError = TEXT("PgSqlConnectorSubsystem is unavailable.");
		return false;
	}

	FString NewConnectionKey;
	if (!ConnectorSubsystem->OpenConnection(Config, NewConnectionKey, OutError))
	{
		return false;
	}

	ConnectionKey = NewConnectionKey;
	bConnected = true;
	return true;
}

void UPgSqlConnectionHandle::Disconnect()
{
	if (!bConnected)
	{
		return;
	}

	if (ConnectorSubsystem != nullptr)
	{
		ConnectorSubsystem->CloseConnection(ConnectionKey);
	}

	ConnectionKey.Reset();
	bConnected = false;
}

bool UPgSqlConnectionHandle::IsConnected() const
{
	return bConnected && ConnectorSubsystem != nullptr && ConnectorSubsystem->IsConnectionOpen(ConnectionKey);
}

FPgSqlQueryResult UPgSqlConnectionHandle::ExecuteQuery(const FString& Sql)
{
	if (!IsConnected())
	{
		return FPgSqlQueryResult::MakeError(TEXT("The connection is not open."));
	}

	return ConnectorSubsystem->ExecuteConnectedQuery(ConnectionKey, Sql);
}

FPgSqlTransactionResult UPgSqlConnectionHandle::ExecuteTransaction(const TArray<FPgSqlStatement>& Statements)
{
	if (!IsConnected())
	{
		return FPgSqlTransactionResult::MakeError(TEXT("The connection is not open."));
	}

	return ConnectorSubsystem->ExecuteConnectedTransaction(ConnectionKey, Statements);
}

TFuture<FPgSqlQueryResult> UPgSqlConnectionHandle::ExecuteQueryFuture(const FString& Sql)
{
	if (!IsConnected())
	{
		TPromise<FPgSqlQueryResult> Promise;
		TFuture<FPgSqlQueryResult> Future = Promise.GetFuture();
		Promise.SetValue(FPgSqlQueryResult::MakeError(TEXT("The connection is not open.")));
		return Future;
	}

	return ConnectorSubsystem->ExecuteConnectedQueryFuture(ConnectionKey, Sql);
}

TFuture<FPgSqlTransactionResult> UPgSqlConnectionHandle::ExecuteTransactionFuture(const TArray<FPgSqlStatement>& Statements)
{
	if (!IsConnected())
	{
		TPromise<FPgSqlTransactionResult> Promise;
		TFuture<FPgSqlTransactionResult> Future = Promise.GetFuture();
		Promise.SetValue(FPgSqlTransactionResult::MakeError(TEXT("The connection is not open.")));
		return Future;
	}

	return ConnectorSubsystem->ExecuteConnectedTransactionFuture(ConnectionKey, Statements);
}

void UPgSqlConnectionHandle::ExecuteQueryAsync(const FString& Sql, FPgSqlQueryNativeDelegate Callback)
{
	if (!IsConnected())
	{
		if (Callback.IsBound())
		{
			Callback.Execute(FPgSqlQueryResult::MakeError(TEXT("The connection is not open.")));
		}
		return;
	}

	ConnectorSubsystem->ExecuteConnectedQueryAsync(ConnectionKey, Sql, MoveTemp(Callback));
}

void UPgSqlConnectionHandle::ExecuteTransactionAsync(const TArray<FPgSqlStatement>& Statements, FPgSqlTransactionNativeDelegate Callback)
{
	if (!IsConnected())
	{
		if (Callback.IsBound())
		{
			Callback.Execute(FPgSqlTransactionResult::MakeError(TEXT("The connection is not open.")));
		}
		return;
	}

	ConnectorSubsystem->ExecuteConnectedTransactionAsync(ConnectionKey, Statements, MoveTemp(Callback));
}
