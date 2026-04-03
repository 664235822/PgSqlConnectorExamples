// Copyright HaoJunDeveloper 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "PgSqlTypes.h"
#include "PgSqlQueryAsyncAction.generated.h"

class UPgSqlConnectionHandle;

UCLASS()
class PGSQLCONNECTORTOOLS_API UPgSqlQueryAsyncAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FPgSqlQueryResultDynamicDelegate OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FPgSqlQueryResultDynamicDelegate OnFailure;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Execute PostgreSQL Query Async"))
	static UPgSqlQueryAsyncAction* ExecutePgSqlQueryAsync(UObject* WorldContextObject, UPgSqlConnectionHandle* Connection, const FString& Sql);

	virtual void Activate() override;

private:
	void HandleCompleted(const FPgSqlQueryResult& Result);

	TWeakObjectPtr<UObject> WorldContextObject;
	UPROPERTY(Transient)
	TObjectPtr<UPgSqlConnectionHandle> Connection = nullptr;
	FString Sql;
};
