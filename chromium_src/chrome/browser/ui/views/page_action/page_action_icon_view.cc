/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"

#include "src/chrome/browser/ui/views/page_action/page_action_icon_view.cc"

void PageActionIconView::SetLoadingIndicator(
    std::unique_ptr<PageActionIconLoadingIndicatorView> indicator) {
  if (loading_indicator_) {
    RemoveChildViewT(loading_indicator_);
  }

  loading_indicator_ = AddChildView(std::move(indicator));
  loading_indicator_->SetVisible(false);
}
