/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_EXTENSIONS_COMMON_CONSTANTS_H_
#define BRAVE_CHROMIUM_SRC_EXTENSIONS_COMMON_CONSTANTS_H_

#include "build/build_config.h"
#include "extensions/buildflags/buildflags.h"

#if (BUILDFLAG(IS_IOS) || BUILDFLAG(IS_ANDROID)) && !BUILDFLAG(ENABLE_DESKTOP_ANDROID_EXTENSIONS)
// ios and android cannot include extensions deps
namespace extensions {
inline constexpr char kExtensionScheme[] = "chrome-extension";
}
#else
#include <extensions/common/constants.h>  // IWYU pragma: export

inline constexpr int kBraveActionLeftMarginExtra = -2;

inline constexpr char brave_extension_id[] = "mnojpmjdmbbfmejpflffifhffcmidifd";
inline constexpr char crl_set_extension_id[] =
    "hfnkpimlhhgieaddgfemjhofmfblmnib";

inline constexpr char google_translate_extension_id[] =
    "aapbdbdomjkkjkaonfhkkikfgjllcleb";
#endif

#endif  // BRAVE_CHROMIUM_SRC_EXTENSIONS_COMMON_CONSTANTS_H_
