// Copyright HaoJunDeveloper 2026. All Rights Reserved.

#include "PgSqlLibpqApi.h"

#include "HAL/PlatformProcess.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"

namespace PgSqlConnectorLibpq
{
	template <typename TFunction>
	bool BindExport(void* ModuleHandle, const TCHAR* ExportName, TFunction& OutFunction)
	{
		OutFunction = reinterpret_cast<TFunction>(FPlatformProcess::GetDllExport(ModuleHandle, ExportName));
		return OutFunction != nullptr;
	}
}

FPgSqlLibpqApi& FPgSqlLibpqApi::Get()
{
	static FPgSqlLibpqApi Instance;
	return Instance;
}

bool FPgSqlLibpqApi::EnsureLoaded(FString& OutError)
{
	FScopeLock Lock(&Mutex);

	if (DllHandle != nullptr)
	{
		OutError.Reset();
		return true;
	}

	for (const FString& CandidatePath : GetCandidateDllPaths())
	{
		if (LoadFromPath(CandidatePath, OutError))
		{
			return true;
		}
	}

	if (OutError.IsEmpty())
	{
		OutError = TEXT("libpq.dll was not found. Place the official PostgreSQL Windows runtime into Binaries/ThirdParty/PostgreSQL/Win64.");
	}

	return false;
}

void FPgSqlLibpqApi::Unload()
{
	FScopeLock Lock(&Mutex);

	if (DllHandle != nullptr)
	{
		FPlatformProcess::FreeDllHandle(DllHandle);
		DllHandle = nullptr;
	}

	ResolvedDllPath.Reset();
	ResetExports();
}

bool FPgSqlLibpqApi::LoadFromPath(const FString& CandidatePath, FString& OutError)
{
	if (CandidatePath.IsEmpty())
	{
		return false;
	}

	void* CandidateHandle = nullptr;

	if (FPaths::IsRelative(CandidatePath))
	{
		CandidateHandle = FPlatformProcess::GetDllHandle(*CandidatePath);
	}
	else if (FPaths::FileExists(CandidatePath))
	{
		CandidateHandle = FPlatformProcess::GetDllHandle(*CandidatePath);
	}

	if (CandidateHandle == nullptr)
	{
		return false;
	}

	DllHandle = CandidateHandle;
	ResolvedDllPath = CandidatePath;

	if (!BindExports(OutError))
	{
		FPlatformProcess::FreeDllHandle(DllHandle);
		DllHandle = nullptr;
		ResolvedDllPath.Reset();
		ResetExports();
		return false;
	}

	OutError.Reset();
	return true;
}

bool FPgSqlLibpqApi::BindExports(FString& OutError)
{
	using namespace PgSqlConnectorLibpq;

	const bool bSucceeded =
		BindExport(DllHandle, TEXT("PQconnectdb"), ConnectDb) &&
		BindExport(DllHandle, TEXT("PQfinish"), Finish) &&
		BindExport(DllHandle, TEXT("PQstatus"), Status) &&
		BindExport(DllHandle, TEXT("PQerrorMessage"), ErrorMessage) &&
		BindExport(DllHandle, TEXT("PQexec"), Exec) &&
		BindExport(DllHandle, TEXT("PQexecParams"), ExecParams) &&
		BindExport(DllHandle, TEXT("PQclear"), Clear) &&
		BindExport(DllHandle, TEXT("PQresultStatus"), ResultStatus) &&
		BindExport(DllHandle, TEXT("PQntuples"), NumTuples) &&
		BindExport(DllHandle, TEXT("PQnfields"), NumFields) &&
		BindExport(DllHandle, TEXT("PQfname"), FieldName) &&
		BindExport(DllHandle, TEXT("PQgetvalue"), GetValue) &&
		BindExport(DllHandle, TEXT("PQgetisnull"), GetIsNull) &&
		BindExport(DllHandle, TEXT("PQcmdStatus"), CmdStatus) &&
		BindExport(DllHandle, TEXT("PQcmdTuples"), CmdTuples) &&
		BindExport(DllHandle, TEXT("PQsetClientEncoding"), SetClientEncoding) &&
		BindExport(DllHandle, TEXT("PQisthreadsafe"), IsThreadSafePtr);

	if (!bSucceeded)
	{
		OutError = FString::Printf(TEXT("libpq.dll loaded but required exports are missing: %s"), *ResolvedDllPath);
		return false;
	}

	if (!IsThreadSafe())
	{
		OutError = FString::Printf(TEXT("The loaded libpq is not thread-safe: %s"), *ResolvedDllPath);
		return false;
	}

	return true;
}

TArray<FString> FPgSqlLibpqApi::GetCandidateDllPaths() const
{
	TArray<FString> Candidates;

	const FString OverridePath = FPlatformMisc::GetEnvironmentVariable(TEXT("PGSQLCONNECTOR_LIBPQ_PATH"));
	if (!OverridePath.IsEmpty())
	{
		Candidates.Add(OverridePath);
	}

	if (const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("PgSqlConnectorTools")))
	{
		const FString PluginBaseDir = Plugin->GetBaseDir();
		Candidates.Add(FPaths::Combine(PluginBaseDir, TEXT("Binaries/ThirdParty/PostgreSQL/Win64/libpq.dll")));
		Candidates.Add(FPaths::Combine(PluginBaseDir, TEXT("ThirdParty/PostgreSQL/Win64/libpq.dll")));
		Candidates.Add(FPaths::Combine(PluginBaseDir, TEXT("Source/ThirdParty/PostgreSQL/Win64/libpq.dll")));
		Candidates.Add(FPaths::Combine(PluginBaseDir, TEXT("../ThirdParty/PgsqlConnectorCpp/lib/libpq.dll")));
	}

	Candidates.Add(TEXT("libpq.dll"));
	return Candidates;
}

void FPgSqlLibpqApi::ResetExports()
{
	ConnectDb = nullptr;
	Finish = nullptr;
	Status = nullptr;
	ErrorMessage = nullptr;
	Exec = nullptr;
	ExecParams = nullptr;
	Clear = nullptr;
	ResultStatus = nullptr;
	NumTuples = nullptr;
	NumFields = nullptr;
	FieldName = nullptr;
	GetValue = nullptr;
	GetIsNull = nullptr;
	CmdStatus = nullptr;
	CmdTuples = nullptr;
	SetClientEncoding = nullptr;
	IsThreadSafePtr = nullptr;
}
