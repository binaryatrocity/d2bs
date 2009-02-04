/*
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

// TODO: Rewrite this crap :(

#define _USE_32BIT_TIME_T

#include "JSDirectory.h"
#include "File.h"

#include "debugnew/debug_new.h"

////////////////////////////////////////////////////////////////////////////////
// Directory stuff
////////////////////////////////////////////////////////////////////////////////

JSBool openDir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	CDebug cDbg("openDir");

	if(argc != 1)
		return JS_TRUE;

	char path[_MAX_PATH];
	char* name = JS_GetStringBytes(JSVAL_TO_STRING(argv[0]));

	if(!isValidPath(name))
		return JS_TRUE;

	sprintf(path, "%s\\%s", Vars.szScriptPath, name);

	if((_mkdir(path) == -1) && (errno == ENOENT))
	{
		JS_ReportError(cx, "Couldn't get directory %s, path '%s' not found", name, path);
		return JS_TRUE;
	}
	else
	{
		DirData* d = new DirData(name);
		JSObject *jsdir = BuildObject(cx, &directory_class, dir_methods, dir_props, d);
		*rval=OBJECT_TO_JSVAL(jsdir);
	}

	return JS_TRUE;
}


////////////////////////////////////////////////////////////////////////////////
// dir_getFiles
// searches a directory for files with a specified extension(*.* assumed)
//
// Added by lord2800
////////////////////////////////////////////////////////////////////////////////

JSBool dir_getFiles(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	CDebug cDbg("dir getFiles");

	if(argc > 1) return JS_TRUE;
	if(argc < 1) argv[0] = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "*.*"));

	DirData* d = (DirData*)JS_GetPrivate(cx, obj);
	char* search = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));

	long hFile;
	char path[_MAX_PATH];
	sprintf(path, "%s\\%s", Vars.szScriptPath, d->name);
	char oldpath[_MAX_PATH];
	_getcwd(oldpath, _MAX_PATH);
	_chdir(path);
	_finddata_t found;
	if((hFile = _findfirst(search, &found)) == -1L)
		return JS_TRUE;
	else
	{
		jsint element=0;
		JSObject *jsarray = JS_NewArrayObject(cx, 0, NULL);
		jsval file;
		if(!(found.attrib & _A_SUBDIR))
		{
			file = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, found.name));
			JS_SetElement(cx, jsarray, element++, &file);
		}
		while(_findnext(hFile, &found) == 0)
		{
			if((found.attrib & _A_SUBDIR)) continue;
			file = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, found.name));
			JS_SetElement(cx, jsarray, element++, &file);
		}
		*rval=OBJECT_TO_JSVAL(jsarray);
	}
	_chdir(oldpath);
	return JS_TRUE;
}

JSBool dir_getDirectories(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	CDebug cDbg("dir getDirectories");

	if(argc > 1) return JS_TRUE;
	if(argc < 1) argv[0] = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "*.*"));

	DirData* d = (DirData*)JS_GetPrivate(cx, obj);
	char* search = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));

	long hFile;
	char path[_MAX_PATH];
	sprintf(path, "%s\\%s", Vars.szScriptPath, d->name);
	char oldpath[_MAX_PATH];
	_getcwd(oldpath, _MAX_PATH);
	_chdir(path);
	_finddata_t found;
	if((hFile = _findfirst(search, &found)) == -1L)
		return JS_TRUE;
	else
	{
		jsint element=0;
		JSObject *jsarray = JS_NewArrayObject(cx, 0, NULL);
		jsval file;
		if(!strcmp(found.name, ".") && !strcmp(found.name, "..") && (found.attrib & _A_SUBDIR))
		{
			file = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, found.name));
			JS_SetElement(cx, jsarray, element++, &file);
		}
		while(_findnext(hFile, &found) == 0)
		{
			if(!strcmp(found.name, "..") || !strcmp(found.name, ".") || !(found.attrib & _A_SUBDIR)) continue;
			file = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, found.name));
			JS_SetElement(cx, jsarray, element++, &file);
		}
		*rval=OBJECT_TO_JSVAL(jsarray);
	}
	_chdir(oldpath);
	return JS_TRUE;
}

JSBool dir_create(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	CDebug cDbg("dir create");

	DirData* d = (DirData*)JS_GetPrivate(cx, obj);
	char path[_MAX_PATH];
	char* name = JS_GetStringBytes(JSVAL_TO_STRING(argv[0]));

	if(!isValidPath(name))
		return JS_TRUE;

	sprintf(path, "%s\\%s\\%s", Vars.szScriptPath, d->name, name);
	if(_mkdir(path) == -1 && (errno == ENOENT))
	{
		JS_ReportError(cx, "Couldn't create directory %s, path %s not found", JS_GetStringBytes(JSVAL_TO_STRING(argv[0])), path);
	}
	else
	{
		DirData* d = new DirData(name);
		JSObject* jsdir = BuildObject(cx, &directory_class, dir_methods, dir_props, d);
		*rval=OBJECT_TO_JSVAL(jsdir);
	}

	return JS_TRUE;
}
JSBool dir_delete(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	CDebug cDbg("dir delete");

	DirData* d = (DirData*)JS_GetPrivate(cx, obj);

	char path[_MAX_PATH];
	sprintf(path, "%s\\%s", Vars.szScriptPath, d->name);

	if(_rmdir(path) == -1)
	{
		// TODO: Make an optional param that specifies recursive delete
		if(errno == ENOTEMPTY)
			JS_ReportError(cx, "Tried to delete %s directory, but it is not empty or is the current working directory", path);
		if(errno == ENOENT)
			JS_ReportError(cx, "Path %s not found", path);
	}
	else
	{
		*rval=BOOLEAN_TO_JSVAL(true);
	}

	return JS_TRUE;
}

JSBool dir_getProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	CDebug cDbg("dir getProperty");

	DirData* d = (DirData*)JS_GetPrivate(cx, obj);

	if(JSVAL_IS_INT(id))
		switch(JSVAL_TO_INT(id))
		{
			case DIR_NAME:
				*vp=STRING_TO_JSVAL(STRING_TO_JSVAL(JS_InternString(cx, d->name)));
				break;
		}
	return JS_TRUE;
}

void dir_finalize(JSContext *cx, JSObject *obj)
{
	DirData* d = (DirData*)JS_GetPrivate(cx, obj);
	if(d) delete d;
}
