#ifndef AREA_H
#define AREA_H

#include "js32.h"
#include "CollisionMap.h"

CLASS_CTOR(area);

void area_finalize(JSContext *cx, JSObject *obj);

JSAPI_PROP(area_getProperty);

static JSClass area_class = {
    "Area",	JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, area_finalize,
    NULL, NULL, NULL, area_ctor
};

enum area_tinyid {
	AUNIT_EXITS,
	AUNIT_NAME,
	AUNIT_X,
	AUNIT_XSIZE,
	AUNIT_Y,
	AUNIT_YSIZE,
	AUNIT_ID
};


static JSPropertySpec area_props[] = {
	{"exits",		AUNIT_EXITS,		JSPROP_PERMANENT_VAR,	area_getProperty},
	{"name",		AUNIT_NAME,			JSPROP_PERMANENT_VAR,	area_getProperty},
	{"x",			AUNIT_X,			JSPROP_PERMANENT_VAR,	area_getProperty},
	{"xsize",		AUNIT_XSIZE,		JSPROP_PERMANENT_VAR,	area_getProperty},
	{"y",			AUNIT_Y,			JSPROP_PERMANENT_VAR,	area_getProperty},
	{"ysize",		AUNIT_YSIZE,		JSPROP_PERMANENT_VAR,	area_getProperty},
	{"id",			AUNIT_ID,			JSPROP_PERMANENT_VAR,	area_getProperty},
	{0},
};

struct myArea {
	DWORD AreaId;
	DWORD Exits;
	LPLevelExit ExitArray[255];
};

#endif