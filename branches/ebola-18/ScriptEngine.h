#pragma once
#ifndef __SCRIPTENGINE_H__
#define __SCRIPTENGINE_H__

#include <list>
#include <map>
#include <string>

#include "js32.h"
#include "Script.h"

typedef std::map<std::string, Script*> ScriptMap;

typedef bool (__fastcall *ScriptCallback)(Script*, void*, uint);

enum EngineState {
	Starting,
	Running,
	Paused,
	Stopping,
	Stopped
};

class ScriptEngine
{
	ScriptEngine(void) {};
	virtual ~ScriptEngine(void) = 0;
	ScriptEngine(const ScriptEngine&);
	ScriptEngine& operator=(const ScriptEngine&);

	static JSRuntime* runtime;
	static ScriptMap scripts;
	static EngineState state;
	static CRITICAL_SECTION lock;

public:
	friend class Script;

	static BOOL Startup(void);
	static void Shutdown(void);
	static EngineState GetState(void) { return state; }

	static void FlushCache(void);

	static Script* CompileFile(const char* file, ScriptState state, bool recompile = false);
	static Script* CompileCommand(const char* command);
	static void DisposeScript(Script* script);

	static void ForEachScript(ScriptCallback callback, void* argv, uint argc);
	static unsigned int GetCount(bool active = true, bool unexecuted = false);

	static JSRuntime* GetRuntime(void) { return runtime; }

	static void StopAll(bool forceStop = false);
	static void ExecEventAsync(char* evtName, AutoRoot** argv, uintN argc);
	static void InitClass(JSContext* context, JSObject* globalObject, JSClass* classp,
							 JSNative ctor, JSFunctionSpec* methods, JSPropertySpec* props,
							 JSFunctionSpec* s_methods, JSPropertySpec* s_props);
	static void DefineConstant(JSContext* context, JSObject* globalObject, const char* name, int value);

	friend JSBool gcCallback(JSContext* cx, JSGCStatus status);
};

// this ForEachScript helper is exposed in case it can be of use somewhere
bool __fastcall ExecEventOnScript(Script* script, void* argv, uint argc);
struct EventHelper
{
	char* evtName;
	AutoRoot** argv;
	uintN argc;
	bool executed;
};

JSBool operationCallback(JSContext* cx);
JSBool contextCallback(JSContext* cx, uintN contextOp);
JSBool gcCallback(JSContext* cx, JSGCStatus status);
void reportError(JSContext *cx, const char *message, JSErrorReport *report);

#endif
