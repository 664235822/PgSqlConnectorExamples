// Copyright HaoJunDeveloper 2026. All Rights Reserved.

#include "PgSqlQueryAsyncAction.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "PgSqlConnectionHandle.h"

UPgSqlQueryAsyncAction* UPgSqlQueryAsyncAction::ExecutePgSqlQueryAsync(UObject* WorldContextObject, UPgSqlConnectionHandle* Connection, const FString& Sql)
{
	UPgSqlQueryAsyncAction* Action = NewObject<UPgSqlQueryAsyncAction>();
	Action->WorldContextObject = WorldContextObject;
	Action->Connection = Connection;
	Action->Sql = Sql;
	return Action;
}

void UPgSqlQueryAsyncAction::Activate()
{
	if (!WorldContextObject.IsValid())
	{
		HandleCompleted(FPgSqlQueryResult::MakeError(TEXT("WorldContextObject is invalid.")));
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject.Get(), EGetWorldErrorMode::ReturnNull);
	UGameInstance* GameInstance = World != nullptr ? World->GetGameInstance() : nullptr;

	if (GameInstance != nullptr)
	{
		RegisterWithGameInstance(GameInstance);
	}

	if (Connection == nullptr || !Connection->IsConnected())
	{
		HandleCompleted(FPgSqlQueryResult::MakeError(TEXT("Failed to get an open PostgreSQL connection.")));
		return;
	}

	Connection->ExecuteQueryAsync(Sql, FPgSqlQueryNativeDelegate::CreateUObject(this, &UPgSqlQueryAsyncAction::HandleCompleted));
}

void UPgSqlQueryAsyncAction::HandleCompleted(const FPgSqlQueryResult& Result)
{
	if (Result.bSucceeded)
	{
		OnSuccess.Broadcast(Result);
	}
	else
	{
		OnFailure.Broadcast(Result);
	}

	SetReadyToDestroy();
}
