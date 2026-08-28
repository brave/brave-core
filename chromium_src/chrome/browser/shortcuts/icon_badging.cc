/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Included first so the upstream ids exist before being redefined below,
// otherwise grit would redefine them with a different value.
#include "chrome/grit/chrome_unscaled_resources.h"

#undef IDR_PRODUCT_LOGO_16_SHORTCUTS
#undef IDR_PRODUCT_LOGO_24_SHORTCUTS
#undef IDR_PRODUCT_LOGO_64_SHORTCUTS
#undef IDR_PRODUCT_LOGO_128_SHORTCUTS

#define IDR_PRODUCT_LOGO_16_SHORTCUTS IDR_BRAVE_PRODUCT_LOGO_16_SHORTCUTS
#define IDR_PRODUCT_LOGO_24_SHORTCUTS IDR_BRAVE_PRODUCT_LOGO_24_SHORTCUTS
#define IDR_PRODUCT_LOGO_64_SHORTCUTS IDR_BRAVE_PRODUCT_LOGO_64_SHORTCUTS
#define IDR_PRODUCT_LOGO_128_SHORTCUTS IDR_BRAVE_PRODUCT_LOGO_128_SHORTCUTS

#include <chrome/browser/shortcuts/icon_badging.cc>

#undef IDR_PRODUCT_LOGO_128_SHORTCUTS
#undef IDR_PRODUCT_LOGO_64_SHORTCUTS
#undef IDR_PRODUCT_LOGO_24_SHORTCUTS
#undef IDR_PRODUCT_LOGO_16_SHORTCUTS
