#ifndef BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_MINI_INSTALLER_REGKEY_H_
#define BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_MINI_INSTALLER_REGKEY_H_

#define WriteDWValue(...) WriteDWValue(__VA_ARGS__); \
  LONG WriteSZValue(const wchar_t* value_name, const wchar_t* value)

#include <chrome/installer/mini_installer/regkey.h>  // IWYU pragma: export

#undef WriteDWValue

#endif  // BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_MINI_INSTALLER_REGKEY_H_
