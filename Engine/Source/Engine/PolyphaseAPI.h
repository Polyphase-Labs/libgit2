#pragma once

/**
 * @file PolyphaseAPI.h
 * @brief Export macros for Polyphase Engine symbols.
 *
 * This header defines POLYPHASE_API which is used to export symbols from the
 * engine executable so that native addon DLLs can link against them.
 *
 * Build Configuration:
 * - When building Polyphase.exe: Define POLYPHASE_ENGINE_EXPORT
 * - When building native addons: Don't define POLYPHASE_ENGINE_EXPORT (symbols imported)
 */

#ifdef _WIN32
    // Suppress C4251: 'type' needs to have dll-interface to be used by clients
    // This warning is about STL types in exported classes. It's safe to suppress
    // because both the engine and addons use the same C++ runtime (MultiThreadedDLL).
    #pragma warning(disable: 4251)
    #pragma warning(disable: 4275)  // Non-dll-interface base class

    #ifdef POLYPHASE_ENGINE_EXPORT
        // Building the engine - export symbols
        #define POLYPHASE_API __declspec(dllexport)
    #else
        // Building a plugin/addon - import symbols from engine
        #define POLYPHASE_API __declspec(dllimport)
    #endif
#else
    // On Linux/other platforms, use visibility attribute
    #define POLYPHASE_API __attribute__((visibility("default")))
#endif

// Convenience macro for classes that should be fully exported
// Use: class POLYPHASE_API MyClass { ... };
// This exports the class's vtable, RTTI, and all members

// For template instantiations, use explicit instantiation:
// extern template class POLYPHASE_API SmartPointer<MyClass>;

// Backwards-compatibility aliases (deprecated, will be removed in a future version)
#ifndef POLYPHASE_NO_LEGACY_MACROS
    #define OCTAVE_API             POLYPHASE_API
    #define OCTAVE_IMGUI_EXPORT    POLYPHASE_IMGUI_EXPORT
    #define OCTAVE_ENGINE_EXPORT   POLYPHASE_ENGINE_EXPORT
    #define OCTAVE_VERSION         POLYPHASE_VERSION
    #define OCTAVE_VERSION_STRING  POLYPHASE_VERSION_STRING
    #define OCTAVE_REGISTER_PLUGIN POLYPHASE_REGISTER_PLUGIN
#endif
