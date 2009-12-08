#pragma once

#include "js32.h"

struct Private { DWORD dwPrivateType; };

JSAPI_FUNC(my_getDistance);
JSAPI_FUNC(my_getPath);
JSAPI_FUNC(my_getCollision);
JSAPI_FUNC(my_getMercHP);
JSAPI_FUNC(my_getCursorType);
JSAPI_FUNC(my_getSkillByName);
JSAPI_FUNC(my_getSkillById);
JSAPI_FUNC(my_getLocaleString);
JSAPI_FUNC(my_getTextWidthHeight);
JSAPI_FUNC(my_getThreadPriority);
JSAPI_FUNC(my_getUIFlag);
JSAPI_FUNC(my_getTradeInfo);
JSAPI_FUNC(my_getWaypoint);
JSAPI_FUNC(my_getScript);
JSAPI_FUNC(my_getRoom);
JSAPI_FUNC(my_getParty);
JSAPI_FUNC(my_getPresetUnit);
JSAPI_FUNC(my_getPresetUnits);
JSAPI_FUNC(my_getArea);
JSAPI_FUNC(my_getExits);
JSAPI_FUNC(my_getBaseStat);
JSAPI_FUNC(my_getControl);
JSAPI_FUNC(my_getPlayerFlag);
JSAPI_FUNC(my_debugLog);

JSAPI_FUNC(my_print);
JSAPI_FUNC(my_delay);
JSAPI_FUNC(my_load);
JSAPI_FUNC(my_include);
JSAPI_FUNC(my_copyUnit);
JSAPI_FUNC(my_stop);
JSAPI_FUNC(my_clickMap);
JSAPI_FUNC(my_acceptTrade);
JSAPI_FUNC(my_beep);
JSAPI_FUNC(my_clickItem);
JSAPI_FUNC(my_rand);
JSAPI_FUNC(my_gold);
JSAPI_FUNC(my_checkCollision);
JSAPI_FUNC(my_isIncluded);
JSAPI_FUNC(my_playSound);
JSAPI_FUNC(my_quit);
JSAPI_FUNC(my_quitGame);
JSAPI_FUNC(my_say);
JSAPI_FUNC(my_sendCopyData);
JSAPI_FUNC(my_sendDDE);
JSAPI_FUNC(my_keystate);
JSAPI_FUNC(my_getTickCount);
JSAPI_FUNC(my_addEventListener);
JSAPI_FUNC(my_removeEventListener);
JSAPI_FUNC(my_clearEvent);
JSAPI_FUNC(my_clearAllEvents);
JSAPI_FUNC(my_clickParty);
JSAPI_FUNC(my_blockMinimize);
JSAPI_FUNC(my_js_strict);
JSAPI_FUNC(my_weaponSwitch);
JSAPI_FUNC(my_version);
JSAPI_FUNC(my_transmute);
JSAPI_FUNC(my_scriptBroadcast);
JSAPI_FUNC(my_iniread);
JSAPI_FUNC(my_iniwrite);
JSAPI_FUNC(my_login);
JSAPI_FUNC(my_createGame);
JSAPI_FUNC(my_joinGame);
JSAPI_FUNC(my_addProfile);
JSAPI_FUNC(my_getOOGLocation);
JSAPI_FUNC(my_getMouseCoords);
JSAPI_FUNC(my_submitItem);
JSAPI_FUNC(my_getInteractedNPC);

