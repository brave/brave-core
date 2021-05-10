/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_new_tab_button.h"
#include "brave/browser/ui/views/tabs/brave_tab_search_button.h"

#define NewTabButton BraveNewTabButton
#define TabSearchButton BraveTabSearchButton
#include "../../../../../../../chrome/browser/ui/views/frame/tab_strip_region_view.cc"
#undef TabSearchButton
#undef NewTabButton
