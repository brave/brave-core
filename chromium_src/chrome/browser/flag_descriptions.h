/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_FLAG_DESCRIPTIONS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_FLAG_DESCRIPTIONS_H_

#include "src/chrome/browser/flag_descriptions.h"  // IWYU pragma: export

#if BUILDFLAG(IS_WIN)
namespace flag_descriptions {
extern const char kImmersiveFullscreenName[];
extern const char kImmersiveFullscreenDescription[];
}  // namespace flag_descriptions
#endif  // BUILDFLAG(IS_WIN)

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_FLAG_DESCRIPTIONS_H_
