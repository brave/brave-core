/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_THEMES_BRAVE_DARK_MODE_UTILS_INTERNAL_H_
#define BRAVE_BROWSER_THEMES_BRAVE_DARK_MODE_UTILS_INTERNAL_H_

namespace dark_mode {

namespace internal {

// Only used by functions in brave_dark_mode_utils.h
// If user choose dark/light explicitly, we should make underlying ui modules
// know about this change. Then, they notify this change to others.
void SetSystemDarkModeForNonDefaultMode(bool dark_mode);

}  // namespace internal

}  // namespace dark_mode

#endif  // BRAVE_BROWSER_THEMES_BRAVE_DARK_MODE_UTILS_INTERNAL_H_
