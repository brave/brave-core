/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_BUILD_ROOT_MENU                     \
  if (!model->other_node()->children().empty() && \
      model->other_node()->IsVisible()) {
#include "../../../../../../../chrome/browser/ui/cocoa/bookmarks/bookmark_menu_bridge.mm"
#undef BRAVE_BUILD_ROOT_MENU
