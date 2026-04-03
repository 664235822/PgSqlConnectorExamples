// Copyright Epic Games, Inc. All Rights Reserved.

#include "PgSqlConnectorTools.h"
#include "PgSqlLibpqApi.h"

#define LOCTEXT_NAMESPACE "FPgSqlConnectorToolsModule"

DEFINE_LOG_CATEGORY(LogPgSqlConnectorTools);

void FPgSqlConnectorToolsModule::StartupModule()
{
}

void FPgSqlConnectorToolsModule::ShutdownModule()
{
	FPgSqlLibpqApi::Get().Unload();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPgSqlConnectorToolsModule, PgSqlConnectorTools)
