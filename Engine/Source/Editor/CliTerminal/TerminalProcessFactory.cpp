#if EDITOR

#include "ITerminalProcess.h"

#if PLATFORM_WINDOWS
#include "TerminalProcess_Windows.h"
#include "TerminalProcess_WindowsConPTY.h"
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

ITerminalProcess* CreateTerminalProcessConPTY()
{
#if PLATFORM_WINDOWS
    if (!TerminalProcess_WindowsConPTY::IsConPtyAvailable())
    {
        return nullptr;
    }
    return new TerminalProcess_WindowsConPTY();
#else
    // POSIX TTY support (forkpty/openpty) is not yet implemented; caller
    // should fall back to CreateTerminalProcess().
    return nullptr;
#endif
}

#endif
