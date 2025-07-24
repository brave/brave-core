/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ui/accessibility/accessibility_features.h"

#include "build/build_config.h"

#if !BUILDFLAG(IS_ANDROID)
#define IsScreenAIOCREnabled IsScreenAIOCREnabled_ChromiumImpl
#define IsScreenAIMainContentExtractionEnabled \
  IsScreenAIMainContentExtractionEnabled_ChromiumImpl
#endif

#include <ui/accessibility/accessibility_features.cc>

#if !BUILDFLAG(IS_ANDROID)
#undef IsScreenAIMainContentExtractionEnabled
#undef IsScreenAIOCREnabled

namespace features {

bool IsScreenAIMainContentExtractionEnabled() {
  return false;
}

bool IsScreenAIOCREnabled() {
  return false;
}

}  // namespace features
#endif  // !BUILDFLAG(IS_ANDROID)
