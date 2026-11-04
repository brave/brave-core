/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/extensions/extensions_toolbar_button.h"

// Include first so the macro below doesn't rewrite the constant's definition.
#include "chrome/browser/ui/layout_constants.h"

// Upstream hardcodes the non-touch size instead of using the layout constant.
#define kDefaultIconSizeChromeRefresh \
  GetLayoutConstant(LayoutConstant::kToolbarButtonIconSize)
#include <chrome/browser/ui/views/extensions/extensions_toolbar_button.cc>
#undef kDefaultIconSizeChromeRefresh
