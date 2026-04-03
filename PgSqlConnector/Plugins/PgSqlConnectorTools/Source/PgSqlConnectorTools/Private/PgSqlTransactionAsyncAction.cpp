// Copyright HaoJunDeveloper 2026. All Rights Reserved.

#include "PgSqlTransactionAsyncAction.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "PgSqlConnectionHandle.h"

UPgSqlTransactionAsyncAction* UPgSqlTransactionAsyncAction::ExecutePgSqlTransactionAsync(UObject* WorldContextObject, UPgSqlConnectionHandle* Connection, const TArray<FPgSqlStatement>& Statements)
{
	UPgSqlTransactionAsyncAction* Action = NewObject<UPgSqlTransactionAsyncAction>();
	Action->WorldContextObject = WorldContextObject;
	Action->Connection = Connection;
	Action->Statements = Statements;
	return Action;
}

void UPgSqlTransactionAsyncAction::Activate()
{
	if (!WorldContextObject.IsValid())
	{
		HandleCompleted(FPgSqlTransactionResult::MakeError(TEXT("WorldContextObject is invalid.")));
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
		HandleCompleted(FPgSqlTransactionResult::MakeError(TEXT("Failed to get an open PostgreSQL connection.")));
		return;
	}

	Connection->ExecuteTransactionAsync(Statements, FPgSqlTransactionNativeDelegate::CreateUObject(this, &UPgSqlTransactionAsyncAction::HandleCompleted));
}

void UPgSqlTransactionAsyncAction::HandleCompleted(const FPgSqlTransactionResult& Result)
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
