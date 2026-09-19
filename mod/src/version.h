#pragma once

// Bump it with the release that goes with it, never on its own.
#define MST_VERSION  "1.0.0"
#define MST_NAME     "Master Stack"
// The game builds the plugin was last checked against. 2.03.00 is exe
// 1.0.0.2944 and 2.02.00 is 1.0.0.2850. The anchor resolves on both, so the
// string names both rather than replacing one. It has to fit
// StackStatus::gameVersion, which is char[16].
#define MST_GAME     "2.02.00/2.03.00"
// Base name of the plugin's files next to it: MasterStack.asi, .ini, .log.
#define MST_FILEBASE L"MasterStack"
#define MST_INI      L"MasterStack.ini"
// The same as narrow text, for the module name another plugin looks us up by.
#define MST_MODULE   "MasterStack.asi"
