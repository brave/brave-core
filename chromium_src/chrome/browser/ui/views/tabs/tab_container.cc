/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_UI_VIEWS_TABS_TAB_CONTAINER_CALCULATE_PREFERRED_SIZE_FOR_VERT_TABS \
  return gfx::Size(120,                                                          \
                   GetLayoutConstant(TAB_HEIGHT) *                               \
                       (tabs_view_model_.view_size() + group_views_.size()));

#include "src/chrome/browser/ui/views/tabs/tab_container.cc"
#undef BRAVE_UI_VIEWS_TABS_TAB_CONTAINER_CALCULATE_PREFERRED_SIZE_FOR_VERT_TABS