/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Temporary fix for https://github.com/brave/brave-browser/issues/35938
#define BRAVE_ACCESSIBILITY_PARENT  \
  if (self == _accessibilityParent) \
    return [super accessibilityParent];
#include "src/content/app_shim_remote_cocoa/render_widget_host_view_cocoa.mm"
#undef BRAVE_ACCESSIBILITY_PARENT
