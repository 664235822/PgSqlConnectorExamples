**Introducing plugins to add dependencies**  
In Project Add the required module to ProjectBuild.cs:  
```  
using UnrealBuildTool;
public class PgSqlConnector : ModuleRules
{
	public PgSqlConnector(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[] { "PgSqlConnectorTools" });
		PublicDependencyModuleNames.AddRange(new string[]
			{ "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "PgSqlConnectorTools" });
	}
}
```  
**Add code to uprojrct**
``` 
{
	"FileVersion": 3,
	"EngineAssociation": "5.7",
	"Category": "",
	"Description": "",
	"Modules": [
		{
			"Name": "PgSqlConnector",
			"Type": "Runtime",
			"LoadingPhase": "Default"
		}
	],
	"Plugins": [
		{
			"Name": "ModelingToolsEditorMode",
			"Enabled": true,
			"TargetAllowList": [
				"Editor"
			]
		},
		{
			"Name": "PgSqlConnectorTools",
			"Enabled": true
		}
	]
}
```
**Init PgSqlConnectorTools Connection**
![Init Connection](https://raw.githubusercontent.com/664235822/PgSqlConnectorExamples/main/Wiki/Init.png)

**Insert,Update,Delete Sql Arrays Blueprint Method**
![Execute Sql Arrays](https://raw.githubusercontent.com/664235822/PgSqlConnectorExamples/main/Wiki/ExecuteSqlArrays.png)

**Query and Query Callback Blueprint Method**
![Query Sql](https://raw.githubusercontent.com/664235822/PgSqlConnectorExamples/main/Wiki/QuerySql.png)

**Close PgsqlConnectorTools when Destroyed**
![Close Connection](https://raw.githubusercontent.com/664235822/PgSqlConnectorExamples/main/Wiki/Close.png)