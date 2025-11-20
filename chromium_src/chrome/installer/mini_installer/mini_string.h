// This file contains code that used to be upstream and had to be restored in
// Brave to support delta updates on Windows until we are on Omaha 4. See:
// github.com/brave/brave-core/pull/31937

#ifndef BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_MINI_INSTALLER_MINI_STRING_H_
#define BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_MINI_INSTALLER_MINI_STRING_H_

#include <chrome/installer/mini_installer/mini_string.h>

namespace mini_installer {

// Case insensitive search of the first occurrence of |find| in |source|.
const wchar_t* SearchStringI(const wchar_t* source, const wchar_t* find);

// Searches for |tag| within |str|.  Returns true if |tag| is found and is
// immediately followed by '-' or is at the end of the string.  If |position|
// is non-nullptr, the location of the tag is returned in |*position| on
// success.
bool FindTagInStr(const wchar_t* str,
                  const wchar_t* tag,
                  const wchar_t** position);

}

#endif  // BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_MINI_INSTALLER_MINI_STRING_H_