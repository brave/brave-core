// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_UI_COLOR_COLOR_PROVIDER_KEY_H_
#define BRAVE_CHROMIUM_SRC_UI_COLOR_COLOR_PROVIDER_KEY_H_

// Add kDarker to SchemeVariant enum. This enum will be used for theme for
// Midnight.
#define kExpressive kExpressive, kDarker

#include <ui/color/color_provider_key.h>  // IWYU pragma: export

#undef kExpressive

#endif  // BRAVE_CHROMIUM_SRC_UI_COLOR_COLOR_PROVIDER_KEY_H_
