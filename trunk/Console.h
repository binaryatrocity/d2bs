#pragma once
#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include "Screenhook.h"
#include "D2BS.h"
#include <vector>
#include <string>
#include <windows.h>

class Console
{
private:
	static bool visible, enabled, initialized;
	static std::vector<std::string> lines;
	static BoxHook* box;
	static TextHook* prompt;
	static TextHook* text;
	static LineHook* cursor;
	static TextHook* lineBuffers[14];
	static unsigned int lineCount;
	static CRITICAL_SECTION lock;

public:
	static void Initialize(void);
	static void Destroy(void);
	static bool IsReady(void) { return initialized; }

	static void Toggle(void) { if(IsEnabled()) Hide(); else Show(); }
	static void ToggleBuffer(void) { if(IsVisible()) HideBuffer(); else ShowBuffer(); }
	static void Hide(void);
	static void HideBuffer(void);
	static void Show(void);
	static void ShowBuffer(void);
	static bool IsVisible(void) { return visible; }
	static bool IsEnabled(void) { return enabled; }

	static void AddKey(unsigned int key);
	static void ExecuteCommand(void);
	static void RemoveLastKey(void);
	static void AddLine(std::string line);
	static void Clear(void);
	static void Draw(void);
};

#endif
