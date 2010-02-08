#include <algorithm>
#include <vector>

#include "ScriptEngine.h"
#include "Core.h"
#include "JSGlobalFuncs.h"
#include "JSGlobalClasses.h"
#include "JSUnit.h"
#include "Constants.h"
#include "D2BS.h"

using namespace std;

JSRuntime* ScriptEngine::runtime = NULL;
ScriptMap ScriptEngine::scripts = ScriptMap();
EngineState ScriptEngine::state = EngineStopped;
CRITICAL_SECTION ScriptEngine::lock = {0};

// internal ForEachScript helpers
bool __fastcall DisposeScript(Script* script, void*, uint);
bool __fastcall StopScript(Script* script, void* argv, uint argc);
bool __fastcall GCPauseScript(Script* script, void* argv, uint argc);

Script* ScriptEngine::CompileFile(const char* file, ScriptType type, bool recompile)
{
	if(GetState() != EngineRunning)
		return NULL;

	EnterCriticalSection(&lock);

	if(!file)
		DebugBreak();

	char* fileName = _strdup(file);
	if(!fileName)
		DebugBreak();

	_strlwr_s(fileName, strlen(fileName)+1);

	Script* script = NULL;

	try
	{
		if(scripts.count(fileName) == 0)
		{
			script = new Script(fileName, type);
			if(!script)
				throw std::exception("Unable to create script object");
			scripts[fileName] = script;
		}
		else
		{
			if(Vars.bDisableCache)
			{
				delete[] fileName;
				fileName = NULL;
				LeaveCriticalSection(&lock);
				return NULL;
			}

			if(recompile)
			{
				scripts[fileName]->Stop();
				DisposeScript(scripts[fileName]);

				script = new Script(fileName, type);
				if(!script)
					throw std::exception("Unable to create script object");
				scripts[fileName] = script;
			}
		}

		delete[] fileName;
		fileName = NULL;
		LeaveCriticalSection(&lock);
		return script;
	}
	catch(std::exception e)
	{
		Print(const_cast<char*>(e.what()));
		delete[] fileName;
		LeaveCriticalSection(&lock);
		return NULL;
	}
}

Script* ScriptEngine::CompileCommand(const char* command)
{
	if(GetState() != EngineRunning)
		return NULL;

	try
	{
		EnterCriticalSection(&lock);
		if(!Vars.bDisableCache)
		{
			if(scripts.count("Command Line") > 0)
			{
				DisposeScript(scripts["Command Line"]);
			}
		}
		Script* script = new Script(command, Command);
		scripts["Command Line"] = script;
		LeaveCriticalSection(&lock);
		return script;
	}
	catch(std::exception e)
	{
		LeaveCriticalSection(&lock);
		Print(const_cast<char*>(e.what()));
		return NULL;
	}
}

void ScriptEngine::DisposeScript(Script* script)
{
	if(GetState() != EngineRunning)
		return;

	EnterCriticalSection(&lock);

#ifdef DEBUG
	if(scripts.count(script->GetFilename()) > 1 || scripts.count(script->GetFilename()) == 0)
		DebugBreak();
#endif

	if(scripts.count(script->GetFilename()))
		scripts.erase(script->GetFilename());

	delete script;
	script = NULL;

	LeaveCriticalSection(&lock);
}

unsigned int ScriptEngine::GetCount(bool active, bool unexecuted)
{
	if(GetState() != EngineRunning)
		return 0;

	EnterCriticalSection(&lock);

	int count = scripts.size();
	if(count)
	{
		for(ScriptMap::iterator it = scripts.begin(); it != scripts.end(); it++)
		{
			if(!active && it->second->GetScriptState() == Running)
				count--;
			if(!unexecuted && it->second->GetExecutionCount() == 0 && it->second->GetScriptState() != Running)
				count--;
		}
	}
	assert(count >= 0);
	LeaveCriticalSection(&lock);

	return count;
}

BOOL ScriptEngine::Startup(void)
{
	if(GetState() == EngineStopped)
	{
		state = EngineStarting;
		InitializeCriticalSection(&lock);
		EnterCriticalSection(&lock);
		// create the runtime with the requested memory limit
		runtime = JS_NewRuntime(Vars.dwMemUsage);
		if(!runtime)
		{
			LeaveCriticalSection(&lock);
			return FALSE;
		}

		//JS_SetContextCallback(runtime, contextCallback);
		//JS_SetGCCallbackRT(runtime, gcCallback);

		state = EngineRunning;
		LeaveCriticalSection(&lock);
	}
	return TRUE;
}

void ScriptEngine::Shutdown(void)
{
	Vars.bActive = FALSE;
	if(GetState() == EngineRunning)
	{
		// bring the engine down properly
		EnterCriticalSection(&lock);
		state = EngineStopping;

		if(scripts.size())
		{
			StopAll(true);

			// clear all scripts now that they're stopped
			ForEachScript(::DisposeScript, NULL, 0);

			scripts.clear();
		}

		if(runtime)
		{
			JS_DestroyRuntime(runtime);
			JS_ShutDown();
			runtime = NULL;
		}

		state = EngineStopped;
		LeaveCriticalSection(&lock);
		DeleteCriticalSection(&lock);
	}
}

void ScriptEngine::StopAll(bool forceStop)
{
	if(GetState() != EngineRunning)
		return;

	if(scripts.size() < 1)
		return;

	EnterCriticalSection(&lock);

	ForEachScript(StopScript, &forceStop, 1);

	LeaveCriticalSection(&lock);
}

void ScriptEngine::FlushCache(void)
{
	if(GetState() != EngineRunning)
		return;

	if(scripts.size() < 1)
		return;

	static bool isFlushing = false;

	if(isFlushing || Vars.bDisableCache)
		return;

	EnterCriticalSection(&lock);
	// TODO: examine if this lock is necessary any more
	EnterCriticalSection(&Vars.cFlushCacheSection);

	isFlushing = true;

	ForEachScript(::DisposeScript, NULL, 0);

	isFlushing = false;

	LeaveCriticalSection(&Vars.cFlushCacheSection);
	LeaveCriticalSection(&lock);
}

void ScriptEngine::ForEachScript(ScriptCallback callback, void* argv, uint argc)
{
	if(callback == NULL || scripts.size() < 1)
		return;

	EnterCriticalSection(&lock);

	// damn std::list not supporting operator[]...
	std::vector<Script*> list;
	for(ScriptMap::iterator it = scripts.begin(); it != scripts.end(); it++)
		list.push_back(it->second);
	int count = list.size();
	// damn std::iterator not supporting manipulating the list...
	for(int i = 0; i < count; i++)
	{
		if(!callback(list[i], argv, argc))
			break;
	}

	LeaveCriticalSection(&lock);
}

void ScriptEngine::ExecEventAsync(char* evtName, AutoRoot** argv, uintN argc)
{
	if(GetState() != EngineRunning)
		return;

	EventHelper helper = {evtName, argv, argc};
	ForEachScript(ExecEventOnScript, &helper, 1);
}

void ScriptEngine::InitClass(JSContext* context, JSObject* globalObject, JSClass* classp,
							 JSNative ctor, JSFunctionSpec* methods, JSPropertySpec* props,
							 JSFunctionSpec* s_methods, JSPropertySpec* s_props)
{
	if(!JS_InitClass(context, globalObject, NULL, classp, ctor, 0, props, methods, s_props, s_methods))
		throw std::exception("Couldn't initialize the class");
}

void ScriptEngine::DefineConstant(JSContext* context, JSObject* globalObject, const char* name, int value)
{
	if(!JS_DefineProperty(context, globalObject, name, INT_TO_JSVAL(value), NULL, NULL, JSPROP_CONSTANT))
		throw std::exception("Couldn't initialize the constant");
}

// internal ForEachScript helper functions
bool __fastcall DisposeScript(Script* script, void*, uint)
{
	ScriptEngine::DisposeScript(script);
	return true;
}

bool __fastcall StopScript(Script* script, void* argv, uint argc)
{
	if(script->GetScriptState() == Running || script->GetScriptState() == Paused)
		script->Stop();
	return true;
}

bool __fastcall ExecEventOnScript(Script* script, void* argv, uint argc)
{
	EventHelper* helper = (EventHelper*)argv;
	script->ExecEventAsync(helper->evtName, helper->argc, helper->argv);
	return true;
}

bool __fastcall GCPauseScript(Script* script, void* argv, uint argc)
{
	ScriptList* list = (ScriptList*)argv;
	if(script->GetScriptState() == Running)
	{
		if(find(list->begin(), list->end(), script) == list->end())
		{
			list->push_back(script);
			script->Pause();
		}
	}
	return true;
}

JSBool operationCallback(JSContext* cx)
{
	Script* script = (Script*)JS_GetContextPrivate(cx);
	if(!script)
	{
		DebugBreak();
		return JS_FALSE;
	}

	//JS_SetOperationCallback(cx, NULL);

	switch(script->GetScriptState())
	{
		case Paused:
			{
				Print("OP %s pause.", script->GetFilename());
				jsrefcount depth = JS_SuspendRequest(cx);
				while(script->GetScriptState() == Paused)
					Sleep(100);
				JS_ResumeRequest(cx, depth);
			}
			break;
		case Stopped:
			{
				Print("OP %s stopped.", script->GetFilename());
				return JS_FALSE;
			}
			break;
		case Running:
			{
				Print("OP %s running.", script->GetFilename());
			}
			break;
	}

	//JS_SetOperationCallback(cx, operationCallback);

	if(ClientState() != ClientStateInGame && script->GetScriptType() == InGame)
	{
		Print("OP %s in game script stopped.", script->GetFilename());
		return JS_FALSE;
	}

	return JS_TRUE;
}

JSBool gcCallback(JSContext *cx, JSGCStatus status)
{
	//EnterCriticalSection(&ScriptEngine::lock);
	//static ScriptList pausedList = ScriptList();
	if(status == JSGC_BEGIN)
	{
		//ScriptEngine::ForEachScript(GCPauseScript, &pausedList, 0);

#ifdef DEBUG
			Log("*** ENTERING GC ***");
#endif
	}
	else if(status == JSGC_END)
	{
#ifdef DEBUG
			Log("*** LEAVING GC ***");
#endif
		//for(ScriptList::iterator it = pausedList.begin(); it != pausedList.end(); it++)
		//	if((*it)->IsPaused())
		//		(*it)->Resume();
		//pausedList.clear();
	}
	//LeaveCriticalSection(&ScriptEngine::lock);
	return JS_TRUE;
}

void reportError(JSContext *cx, const char *message, JSErrorReport *report)
{
	bool warn = JSREPORT_IS_WARNING(report->flags);
	if(Vars.bQuitOnError && D2CLIENT_GetPlayerUnit() && !warn)
		D2CLIENT_ExitGame();

	bool isStrict = JSREPORT_IS_STRICT(report->flags);
	const char* type = (warn ? "Warning" : "Error");
	const char* strict = (isStrict ? "Strict " : "");
	char* filename = NULL;
	filename = (report->filename ? _strdup(report->filename) : _strdup("<unknown>"));

	Log("[%s%s] Code(%d) File(%s:%d) %s\nLine: %s", 
			strict, type, report->errorNumber, filename, report->lineno, message, report->linebuf);

	Print("[�c%d%s%s�c0 (%d)] File(%s:%d) %s", 
			(warn ? 9 : 1), strict, type, report->errorNumber, filename, report->lineno, message);

	if(filename)
	{
		free(filename);
		filename = NULL;
	}
}