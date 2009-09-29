#include "ScriptEngine.h"
#include "D2BS.h"
#include "Core.h"
#include "JSGlobalFuncs.h"
#include "JSUnit.h"
#include "Constants.h"

#include <algorithm>

using namespace std;

JSRuntime* ScriptEngine::runtime = NULL;
ScriptMap ScriptEngine::scripts = ScriptMap();
EngineState ScriptEngine::state = Stopped;
CRITICAL_SECTION ScriptEngine::lock = {0};

Script* ScriptEngine::CompileFile(const char* file, ScriptState state, bool recompile)
{
	if(GetState() != Running)
		return NULL;
	file = _strlwr(_strdup(file));
	try {
		EnterCriticalSection(&lock);
		if(!Vars.bDisableCache) {
			if(recompile && scripts.count(file) > 0) {
				scripts[file]->Stop(true, true);
				DisposeScript(scripts[file]);
			} else if(scripts.count(file) > 0) {
				Script* ret = scripts[file];
				ret->Stop(true, true);
				delete[] file;
				return ret;
			}
		}
		Script* script = new Script(file, state);
		scripts[file] = script;
		LeaveCriticalSection(&lock);
		delete[] file;
		return script;
	} catch(std::exception e) {
		LeaveCriticalSection(&lock);
		Print(const_cast<char*>(e.what()));
		delete[] file;
		return NULL;
	}
}

Script* ScriptEngine::CompileCommand(const char* command)
{
	if(GetState() != Running)
		return NULL;
	char* file = _strlwr(_strdup(command));
	try {
		EnterCriticalSection(&lock);
		if(!Vars.bDisableCache) {
			if(scripts.count(file) > 0) {
				Script* ret = scripts[file];
				delete[] file;
				return ret;
			}
		}
		Script* script = new Script(command, Command);
		scripts[file] = script;
		LeaveCriticalSection(&lock);
		delete[] file;
		return script;
	} catch(std::exception e) {
		LeaveCriticalSection(&lock);
		delete[] file;
		Print(const_cast<char*>(e.what()));
		return NULL;
	}
}

void ScriptEngine::DisposeScript(Script* script)
{
	EnterCriticalSection(&lock);

	if(scripts.count(script->GetFilename()))
		scripts.erase(script->GetFilename());
	delete script;

	LeaveCriticalSection(&lock);
}

unsigned int ScriptEngine::GetCount(bool active, bool unexecuted)
{
	if(GetState() != Running)
		return -1;

	EnterCriticalSection(&lock);
	ScriptList list;
	GetScripts(list);
	int count = list.size();
	for(ScriptList::iterator it = list.begin(); it != list.end(); it++)
	{
		if(!active && (*it)->IsRunning() && !(*it)->IsAborted())
			count--;
		if(!unexecuted && (*it)->GetExecutionCount() == 0)
			count--;
	}
	assert(count >= 0);
	LeaveCriticalSection(&lock);
	return count;
}

BOOL ScriptEngine::Startup(void)
{
	if(GetState() == Stopped)
	{
		state = Starting;
		InitializeCriticalSection(&lock);
		EnterCriticalSection(&lock);
		// create the runtime with the requested memory limit
		runtime = JS_NewRuntime(Vars.dwMemUsage);
		if(!runtime)
		{
			LeaveCriticalSection(&lock);
			return FALSE;
		}
		JS_SetContextCallback(runtime, contextCallback);
		JS_SetGCCallbackRT(runtime, gcCallback);

		//JS_SetThrowHook(runtime, exceptionCallback, NULL);
		//JS_SetCallHook(runtime, executeCallback, NULL);
		//JS_SetExecuteHook(runtime, executeCallback, NULL);
		//JS_SetDebuggerHandler(runtime, debuggerCallback, NULL);

		state = Running;
		LeaveCriticalSection(&lock);
	}
	return TRUE;
}

void ScriptEngine::Shutdown(void)
{
	if(GetState() == Running)
	{
		// bring the engine down properly
		EnterCriticalSection(&lock);
		state = Stopping;
		StopAll(true);

		// clear all scripts now that they're stopped
		ScriptList list;
		GetScripts(list);
		for(ScriptList::iterator it = list.begin(); it != list.end(); it++)
			DisposeScript(*it);

		scripts.clear();

		if(runtime)
		{
			JS_DestroyRuntime(runtime);
			JS_ShutDown();
			runtime = NULL;
		}

		LeaveCriticalSection(&lock);
		DeleteCriticalSection(&lock);
		state = Stopped;
	}
}

void ScriptEngine::StopAll(bool forceStop)
{
	if(GetState() != Running)
		return;

	EnterCriticalSection(&lock);

	ScriptList list;
	GetScripts(list);
	for(ScriptList::iterator it = list.begin(); it != list.end(); it++)
	{
		(*it)->Stop(forceStop, (ScriptEngine::GetState() == Stopping));
	}

	LeaveCriticalSection(&lock);
}

void ScriptEngine::PauseAll(void)
{
	if(GetState() != Running)
		return;

	EnterCriticalSection(&lock);

	ScriptList list;
	GetScripts(list);
	for(ScriptList::iterator it = list.begin(); it != list.end(); it++)
	{
		(*it)->Pause();
	}

	LeaveCriticalSection(&lock);
}

void ScriptEngine::ResumeAll(void)
{
	if(GetState() != Running)
		return;

	EnterCriticalSection(&lock);

	ScriptList list;
	GetScripts(list);
	for(ScriptList::iterator it = list.begin(); it != list.end(); it++)
	{
		(*it)->Resume();
	}

	LeaveCriticalSection(&lock);
}

void ScriptEngine::FlushCache(void)
{
	if(GetState() != Running)
		return;

	static bool isFlushing = false;

	if(isFlushing || Vars.bDisableCache)
		return;

	EnterCriticalSection(&lock);
	// TODO: examine if this lock is necessary any more
	EnterCriticalSection(&Vars.cFlushCacheSection);

	isFlushing = true;

	ScriptList list;
	GetScripts(list);
	for(ScriptList::iterator it = list.begin(); it != list.end(); it++)
	{
		if(!(*it)->IsRunning())
		{
			DisposeScript(*it);
		}
	}

	isFlushing = false;

	LeaveCriticalSection(&Vars.cFlushCacheSection);
	LeaveCriticalSection(&lock);
}

void ScriptEngine::GetScripts(ScriptList& list)
{
	EnterCriticalSection(&lock);

	list.clear();
	for(ScriptMap::iterator it = scripts.begin(); it != scripts.end(); it++)
		list.push_back(it->second);

	LeaveCriticalSection(&lock);
}

#if 0
// Disabled until decision is made to deorbit.
void ScriptEngine::ExecEvent(char* evtName, AutoRoot** argv, uintN argc)
{
	if(GetState() != Running)
		return;

	jsval dummy;
	ScriptList list;
	GetScripts(list);
	for(ScriptList::iterator it = list.begin(); it != list.end(); it++)
		if(!(*it)->IsAborted())
			(*it)->ExecEvent(evtName, argc, argv, &dummy);
}
#endif

void ScriptEngine::ExecEventAsync(char* evtName, AutoRoot** argv, uintN argc)
{
	if(GetState() != Running)
		return;

	ScriptList list;
	GetScripts(list);
	for(ScriptList::iterator it = list.begin(); it != list.end(); it++)
		if(!(*it)->IsAborted())
			(*it)->ExecEventAsync(evtName, argc, argv);
}

void ScriptEngine::InitClass(JSContext* context, JSObject* globalObject, JSClass* classp,
		JSFunctionSpec* methods, JSPropertySpec* props, 
		JSFunctionSpec* s_methods, JSPropertySpec* s_props)
{
	if(!JS_InitClass(context, globalObject, NULL, classp, 
			classp->construct, 0, props, methods, 
			s_props, s_methods))
		throw std::exception("Couldn't initialize the class");
}

void ScriptEngine::DefineConstant(JSContext* context, JSObject* globalObject, const char* name, int value)
{
	if(!JS_DefineProperty(context, globalObject, name, INT_TO_JSVAL(value), NULL, NULL, JSPROP_CONSTANT))
		throw std::exception("Couldn't initialize the constant");
}

JSTrapStatus exceptionCallback(JSContext *cx, JSScript *script, jsbytecode *pc, jsval *rval, void *closure)
{
	return JSTRAP_CONTINUE;
}

void* executeCallback(JSContext* cx, JSStackFrame* frame, JSBool before, JSBool* ok, void* closure)
{
	Script* script = (Script*)JS_GetContextPrivate(cx);

	if(!script)
		return NULL;

	static JSBool dummy;
	dummy = (script->IsSingleStep() ? JS_TRUE : JS_FALSE);
	return dummy ? &dummy : NULL;
}

JSTrapStatus debuggerCallback(JSContext *cx, JSScript *jsscript, jsbytecode *pc, jsval *rval, void *closure)
{
	Script* script = (Script*)JS_GetContextPrivate(cx);

	if(!script)
		return JSTRAP_CONTINUE;

	script->EnableSingleStep();
	return JSTRAP_CONTINUE;
}

JSBool branchCallback(JSContext* cx, JSScript*)
{
	Script* script = (Script*)JS_GetContextPrivate(cx);

	bool pause = script->IsPaused();

	jsrefcount depth = JS_SuspendRequest(cx);

	if(pause)
		script->SetPauseState(true);
	while(script->IsPaused())
		Sleep(50);
	if(pause)
		script->SetPauseState(false);

	JS_ResumeRequest(cx, depth);

	return !!!(JSBool)(script->IsAborted() || ((script->GetState() != OutOfGame) && !D2CLIENT_GetPlayerUnit()));
}

JSBool contextCallback(JSContext* cx, uintN contextOp)
{
	if(contextOp == JSCONTEXT_NEW)
	{
		JS_SetErrorReporter(cx, reportError);
		JS_SetBranchCallback(cx, branchCallback);
		JS_SetOptions(cx, JSOPTION_STRICT|JSOPTION_VAROBJFIX|JSOPTION_XML|JSOPTION_NATIVE_BRANCH_CALLBACK);
		JS_SetVersion(cx, JSVERSION_1_7);

		JSObject* globalObject = JS_NewObject(cx, &global_obj, NULL, NULL);
		if(!globalObject)
			return JS_FALSE;

		if(JS_InitStandardClasses(cx, globalObject) == JS_FALSE)
			return JS_FALSE;
		if(JS_DefineFunctions(cx, globalObject, global_funcs) == JS_FALSE)
			return JS_FALSE;

		// MOVED from Script::ctor
		myUnit* lpUnit = new myUnit;
		memset(lpUnit, NULL, sizeof(myUnit));

		UnitAny* player = D2CLIENT_GetPlayerUnit();
		lpUnit->dwMode = -1;
		lpUnit->dwClassId = -1;
		lpUnit->dwType = UNIT_PLAYER;
		lpUnit->dwUnitId = player ? player->dwUnitId : NULL;
		lpUnit->_dwPrivateType = PRIVATE_UNIT;

		// TODO: turn this into a loop or something
		ScriptEngine::InitClass(cx, globalObject, &file_class_ex.base, file_methods, file_props, file_s_methods, NULL);
		ScriptEngine::InitClass(cx, globalObject, &filetools_class, NULL, NULL, filetools_s_methods, NULL);
		ScriptEngine::InitClass(cx, globalObject, &sqlite_db_ex.base, sqlite_methods, sqlite_props, NULL, NULL);
		ScriptEngine::InitClass(cx, globalObject, &sandbox_class, sandbox_methods, NULL, NULL, NULL);
		ScriptEngine::InitClass(cx, globalObject, &frame_class, frame_methods, frame_props, NULL, NULL);
		ScriptEngine::InitClass(cx, globalObject, &box_class, box_methods, box_props, NULL, NULL);
		ScriptEngine::InitClass(cx, globalObject, &line_class, line_methods, line_props, NULL, NULL);
		ScriptEngine::InitClass(cx, globalObject, &text_class, text_methods, text_props, NULL, NULL);
		ScriptEngine::InitClass(cx, globalObject, &image_class, image_methods, image_props, NULL, NULL);
		ScriptEngine::InitClass(cx, globalObject, &unit_class, unit_methods, unit_props, NULL, NULL);

		JSObject* meObject = BuildObject(cx, &unit_class, unit_methods, me_props, lpUnit);
		if(!meObject)
			return JS_FALSE;

		if(JS_AddNamedRoot(cx, &meObject, "me object") == JS_FALSE)
			return JS_FALSE;
		
		if(JS_DefineProperty(cx, globalObject, "me", OBJECT_TO_JSVAL(meObject), NULL, NULL, JSPROP_CONSTANT) == JS_FALSE)
			return JS_FALSE;
		
#define DEFCONST(vp) ScriptEngine::DefineConstant(cx, globalObject, #vp, vp)
#define DEFEVENT(vp) DEFCONST(EVENT_##vp)
		DEFCONST(FILE_READ);
		DEFCONST(FILE_WRITE);
		DEFCONST(FILE_APPEND);
		
		DEFEVENT(AREACHANGE);
		DEFEVENT(CHATMSG);
		DEFEVENT(COPYDATA);
		DEFEVENT(GAMEMSG);
		DEFEVENT(HOSTILEMSG);
		DEFEVENT(INPUTLINE);
		DEFEVENT(ITEMSTAT);
		DEFEVENT(KEYDOWN);
		DEFEVENT(KEYUP);
		DEFEVENT(MELIFE);
		DEFEVENT(MISSILEMOVE);
		DEFEVENT(MISSILESTATE);
		DEFEVENT(MOUSEDOWN);
		DEFEVENT(MOUSEUP);
		DEFEVENT(NEWITEM);
		DEFEVENT(NEWNPC);
		DEFEVENT(NPCLIFE);
		DEFEVENT(NPCMOVE);
		DEFEVENT(NPCSTAT);
		DEFEVENT(NPCSTATE);
		DEFEVENT(PARTYMSG);
		DEFEVENT(PLAYERMOVE);
		DEFEVENT(PLAYERSTAT);
		DEFEVENT(PLAYERSTATE);
		DEFEVENT(QUEST);
		DEFEVENT(SCRIPTMSG);
		DEFEVENT(UNITMOVE);
		DEFEVENT(WINDOWFOCUS);
		DEFEVENT(CHATCMD);
		DEFEVENT(PLAYERASSIGN);
#undef DEFEVENT
#undef DEFCONST

	}
	return JS_TRUE;
}

JSBool gcCallback(JSContext *cx, JSGCStatus status)
{
	if(status == JSGC_BEGIN)
	{
		ScriptEngine::PauseAll();
		if(Vars.bDebug)
			Log("*** ENTERING GC ***");
	}
	else if(status == JSGC_END)
	{
		if(Vars.bDebug)
			Log("*** LEAVING GC ***");
		ScriptEngine::ResumeAll();
	}
	return JS_TRUE;
}

void reportError(JSContext *cx, const char *message, JSErrorReport *report)
{
	bool warn = JSREPORT_IS_WARNING(report->flags);
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
		free(filename);

	if(Vars.bQuitOnError && D2CLIENT_GetPlayerUnit() && !JSREPORT_IS_WARNING(report->flags))
		D2CLIENT_ExitGame();
}

