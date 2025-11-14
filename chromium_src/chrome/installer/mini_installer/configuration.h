#ifndef BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_MINI_INSTALLER_CONFIGURATION_H_
#define BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_MINI_INSTALLER_CONFIGURATION_H_

#include <windows.h>

#define Initialize() Initialize(HMODULE module)
#define program() \
    previous_version() const { return previous_version_; } \
    const wchar_t* program()
#define Clear() \
    Clear(); \
    void ReadResources(HMODULE module); \
    const wchar_t* previous_version_
#include <chrome/installer/mini_installer/configuration.h>  // IWYU pragma: export
#undef Clear
#undef program
#undef Initialize

#endif  // BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_MINI_INSTALLER_CONFIGURATION_H_
