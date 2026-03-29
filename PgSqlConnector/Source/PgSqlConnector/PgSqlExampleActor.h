#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PgSqlExampleActor.generated.h"

class UPgSqlConnectionHandle;

UCLASS()
class PGSQLCONNECTOR_API APgSqlExampleActor : public AActor
{
	GENERATED_BODY()

public:
	APgSqlExampleActor();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "PgSql|Example")
	bool ConnectToDatabase(FString& OutError);

	UFUNCTION(BlueprintCallable, Category = "PgSql|Example")
	void DisconnectFromDatabase();

	UFUNCTION(BlueprintCallable, Category = "PgSql|Example")
	void RunHelloDemo();

	UFUNCTION(BlueprintCallable, Category = "PgSql|Example")
	FString QueryHelloTable();

	UFUNCTION(BlueprintCallable, Category = "PgSql|Example")
	bool InsertHelloName(const FString& InName, FString& OutError);

protected:
	UPROPERTY(EditAnywhere, Category = "PgSql|Example")
	bool bRunDemoOnBeginPlay = true;

	UPROPERTY(EditAnywhere, Category = "PgSql|Example")
	FString DemoInsertName = TEXT("CodexDemo");

	UPROPERTY(EditAnywhere, Category = "PgSql|Example")
	FString Host = TEXT("127.0.0.1");

	UPROPERTY(EditAnywhere, Category = "PgSql|Example")
	int32 Port = 5432;

	UPROPERTY(EditAnywhere, Category = "PgSql|Example")
	FString Database = TEXT("postgres");

	UPROPERTY(EditAnywhere, Category = "PgSql|Example")
	FString User = TEXT("postgres");

	UPROPERTY(EditAnywhere, Category = "PgSql|Example")
	FString Password = TEXT("zhuzhou9uu897@");

	UPROPERTY(EditAnywhere, Category = "PgSql|Example")
	FString PoolName = TEXT("GameMain");

private:
	void PrintStatus(const FString& Message, const FColor& Color = FColor::Green) const;
	FString EscapeSqlLiteral(const FString& Value) const;
	FString FormatQueryResultRows(const struct FPgSqlQueryResult& Result) const;
	UPgSqlConnectionHandle* GetOrCreateConnection(FString& OutError);

	UPROPERTY(Transient)
	TObjectPtr<UPgSqlConnectionHandle> Connection = nullptr;
};
