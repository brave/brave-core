/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// This file contains code that used to be upstream and had to be restored in
// Brave to support delta updates on Windows until we are on Omaha 4. See:
// github.com/brave/brave-core/pull/31937

#ifndef BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_MINI_INSTALLER_MINI_STRING_H_
#define BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_MINI_INSTALLER_MINI_STRING_H_

#include <chrome/installer/mini_installer/mini_string.h>  // IWYU pragma: export

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

}  // namespace mini_installer

#endif  // BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_MINI_INSTALLER_MINI_STRING_H_
