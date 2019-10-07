/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_INIT other_bookmarks_button_->SetVisible(false);
#define BRAVE_UPDATE_OTHER_AND_MANAGED_BUTTONS_VISIBILITY update_other = false;
#include "../../../../../../../chrome/browser/ui/views/bookmarks/bookmark_bar_view.cc" // NOLINT
#undef BRAVE_UPDATE_OTHER_AND_MANAGED_BUTTONS_VISIBILITY
#undef BRAVE_INIT
