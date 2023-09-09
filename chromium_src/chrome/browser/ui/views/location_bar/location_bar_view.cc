/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/containers/adapters.h"
#include "brave/browser/ui/omnibox/brave_omnibox_client_impl.h"
#include "brave/browser/ui/views/omnibox/brave_omnibox_view_views.h"
#include "brave/browser/ui/views/page_action/brave_page_action_icon_container_view.h"
#include "chrome/browser/ui/views/omnibox/omnibox_view_views.h"

#define BRAVE_LAYOUT_TRAILING_DECORATIONS                                    \
  auto right_most = GetTrailingViews();                                      \
  for (auto* item : base::Reversed(right_most)) {                            \
    if (item->GetVisible())                                                  \
      trailing_decorations.AddDecoration(vertical_padding, location_height,  \
                                         false, 0, /*intra_item_padding=*/0, \
                                         0, item);                           \
  }

#define OmniboxViewViews BraveOmniboxViewViews
#define ChromeOmniboxClient BraveOmniboxClientImpl
#define PageActionIconContainerView BravePageActionIconContainerView
#include "src/chrome/browser/ui/views/location_bar/location_bar_view.cc"
#undef PageActionIconContainerView
#undef ChromeOmniboxClient
#undef OmniboxViewViews
#undef BRAVE_LAYOUT_TRAILING_DECORATIONS

std::vector<views::View*> LocationBarView::GetTrailingViews() {
  return std::vector<views::View*>();
}
