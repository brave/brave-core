/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"
#include "ui/base/ui_base_features.h"

#define BRAVE_TAB_SEARCH_BUBBLE_HOST_SHOULD_TAB_SEARCH_RENDER_BEFORE_TAB_STRIP \
  return false;
#include "src/chrome/browser/ui/views/tab_search_bubble_host.cc"
#undef BRAVE_TAB_SEARCH_BUBBLE_HOST_SHOULD_TAB_SEARCH_RENDER_BEFORE_TAB_STRIP
