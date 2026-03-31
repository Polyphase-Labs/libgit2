#pragma once

#include "PolyphaseAPI.h"
#include <stdarg.h>
#include <stdio.h>
#include "Maths.h"
#include "Constants.h"

void InitializeLog();
void ShutdownLog();

POLYPHASE_API void EnableLog(bool enable);
POLYPHASE_API bool IsLogEnabled();

void LockLog();
void UnlockLog();

POLYPHASE_API void LogDebug(const char* format, ...);
POLYPHASE_API void LogWarning(const char* format, ...);
POLYPHASE_API void LogError(const char* format, ...);
POLYPHASE_API void LogConsole(glm::vec4 color, const char* format, ...);

#include "System/SystemTypes.h"
typedef void(*LogCallbackFP)(LogSeverity severity, const char* message);
void RegisterLogCallback(LogCallbackFP callback);
void UnregisterLogCallback(LogCallbackFP callback);
void SetDebugLogsInBuildEnabled(bool enabled);
bool IsDebugLogsInBuildEnabled();
