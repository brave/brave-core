/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_COMMON_CHROME_FEATURES_H_
#define BRAVE_CHROMIUM_SRC_CHROME_COMMON_CHROME_FEATURES_H_

#include "src/chrome/common/chrome_features.h"  // IWYU pragma: export

#if BUILDFLAG(IS_WIN)
#include "base/component_export.h"
#include "build/buildflag.h"

namespace features {
COMPONENT_EXPORT(CHROME_FEATURES) BASE_DECLARE_FEATURE(kImmersiveFullscreen);
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kImmersiveFullscreenTabs);
COMPONENT_EXPORT(CHROME_FEATURES)
BASE_DECLARE_FEATURE(kImmersiveFullscreenPWAs);
}  // namespace features
#endif  // BUILDFLAG(IS_WIN)

#endif  // BRAVE_CHROMIUM_SRC_CHROME_COMMON_CHROME_FEATURES_H_
