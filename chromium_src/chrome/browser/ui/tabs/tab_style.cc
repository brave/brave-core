/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_tab_style.h"

#define BRAVE_TAB_STYLE_GET \
  return new BraveTabStyle<ChromeRefresh2023TabStyle>();
#include "src/chrome/browser/ui/tabs/tab_style.cc"
#undef BRAVE_TAB_STYLE_GET
