// Copyright HaoJunDeveloper 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct pg_conn;
struct pg_result;

using PGconn = pg_conn;
using PGresult = pg_result;
using Oid = unsigned int;

class FPgSqlLibpqApi
{
public:
	using PQconnectdbFn = PGconn* (*)(const char*);
	using PQfinishFn = void (*)(PGconn*);
	using PQstatusFn = int (*)(const PGconn*);
	using PQerrorMessageFn = char* (*)(const PGconn*);
	using PQexecFn = PGresult* (*)(PGconn*, const char*);
	using PQexecParamsFn = PGresult* (*)(PGconn*, const char*, int, const Oid*, const char* const*, const int*, const int*, int);
	using PQclearFn = void (*)(PGresult*);
	using PQresultStatusFn = int (*)(const PGresult*);
	using PQntuplesFn = int (*)(const PGresult*);
	using PQnfieldsFn = int (*)(const PGresult*);
	using PQfnameFn = char* (*)(const PGresult*, int);
	using PQgetvalueFn = char* (*)(const PGresult*, int, int);
	using PQgetisnullFn = int (*)(const PGresult*, int, int);
	using PQcmdStatusFn = char* (*)(PGresult*);
	using PQcmdTuplesFn = char* (*)(PGresult*);
	using PQsetClientEncodingFn = int (*)(PGconn*, const char*);
	using PQisthreadsafeFn = int (*)();

	static FPgSqlLibpqApi& Get();

	bool EnsureLoaded(FString& OutError);
	void Unload();

	bool IsLoaded() const { return DllHandle != nullptr; }
	bool IsThreadSafe() const { return IsThreadSafePtr != nullptr && IsThreadSafePtr() == 1; }
	const FString& GetResolvedDllPath() const { return ResolvedDllPath; }

	PQconnectdbFn ConnectDb = nullptr;
	PQfinishFn Finish = nullptr;
	PQstatusFn Status = nullptr;
	PQerrorMessageFn ErrorMessage = nullptr;
	PQexecFn Exec = nullptr;
	PQexecParamsFn ExecParams = nullptr;
	PQclearFn Clear = nullptr;
	PQresultStatusFn ResultStatus = nullptr;
	PQntuplesFn NumTuples = nullptr;
	PQnfieldsFn NumFields = nullptr;
	PQfnameFn FieldName = nullptr;
	PQgetvalueFn GetValue = nullptr;
	PQgetisnullFn GetIsNull = nullptr;
	PQcmdStatusFn CmdStatus = nullptr;
	PQcmdTuplesFn CmdTuples = nullptr;
	PQsetClientEncodingFn SetClientEncoding = nullptr;
	PQisthreadsafeFn IsThreadSafePtr = nullptr;

private:
	bool LoadFromPath(const FString& CandidatePath, FString& OutError);
	bool BindExports(FString& OutError);
	TArray<FString> GetCandidateDllPaths() const;
	void ResetExports();

	FCriticalSection Mutex;
	void* DllHandle = nullptr;
	FString ResolvedDllPath;
};
