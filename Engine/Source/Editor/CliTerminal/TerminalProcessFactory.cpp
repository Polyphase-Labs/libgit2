#if EDITOR

#include "ITerminalProcess.h"

#if PLATFORM_WINDOWS
#include "TerminalProcess_Windows.h"
#elif PLATFORM_LINUX
#include "TerminalProcess_Posix.h"
#endif

ITerminalProcess* CreateTerminalProcess()
{
#if PLATFORM_WINDOWS
    return new TerminalProcess_Windows();
#elif PLATFORM_LINUX
    return new TerminalProcess_Posix();
#else
    return nullptr;
#endif
}

#endif
