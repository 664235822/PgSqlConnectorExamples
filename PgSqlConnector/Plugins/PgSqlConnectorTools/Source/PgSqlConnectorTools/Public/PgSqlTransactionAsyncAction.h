// Copyright HaoJunDeveloper 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "PgSqlTypes.h"
#include "PgSqlTransactionAsyncAction.generated.h"

class UPgSqlConnectionHandle;

UCLASS()
class PGSQLCONNECTORTOOLS_API UPgSqlTransactionAsyncAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FPgSqlTransactionResultDynamicDelegate OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FPgSqlTransactionResultDynamicDelegate OnFailure;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Execute PostgreSQL Transaction Async"))
	static UPgSqlTransactionAsyncAction* ExecutePgSqlTransactionAsync(UObject* WorldContextObject, UPgSqlConnectionHandle* Connection, const TArray<FPgSqlStatement>& Statements);

	virtual void Activate() override;

private:
	void HandleCompleted(const FPgSqlTransactionResult& Result);

	TWeakObjectPtr<UObject> WorldContextObject;
	UPROPERTY(Transient)
	TObjectPtr<UPgSqlConnectionHandle> Connection = nullptr;
	TArray<FPgSqlStatement> Statements;
};
