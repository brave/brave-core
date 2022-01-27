/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_SHARING_HUB_BUBBLE_VIEW_IMPL_POPULATE_SCROLL_VIEW \
  share_link_label_ = new views::Label(                         \
      l10n_util::GetStringUTF16(IDS_SHARING_HUB_SHARE_LABEL));  \
  MaybeSizeToContents();                                        \
  Layout();                                                     \
  return;

#include "src/chrome/browser/ui/views/sharing_hub/sharing_hub_bubble_view_impl.cc"
#undef BRAVE_SHARING_HUB_BUBBLE_VIEW_IMPL_POPULATE_SCROLL_VIEW
