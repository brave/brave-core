/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/omnibox/brave_omnibox_client_impl.h"
#include "brave/browser/ui/views/omnibox/brave_omnibox_view_views.h"

#define BRAVE_LAYOUT_TRAILING_DECORATIONS     \
  if (right_most && right_most->GetVisible()) \
    trailing_decorations.AddDecoration(0, height(), false, 0, 0, right_most);

#define ChromeOmniboxClient BraveOmniboxClientImpl
#define OmniboxViewViews BraveOmniboxViewViews
#include "../../../../../../../chrome/browser/ui/views/location_bar/location_bar_view.cc"
#undef OmniboxViewViews
#undef ChromeOmniboxClient
#undef BRAVE_LAYOUT_TRAILING_DECORATIONS
