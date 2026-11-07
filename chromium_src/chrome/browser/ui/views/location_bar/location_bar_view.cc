/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/location_bar/location_bar_view.h"

#include "brave/browser/ui/omnibox/brave_omnibox_client_impl.h"
#include "brave/browser/ui/views/omnibox/brave_omnibox_popup_view_views.h"
#include "brave/browser/ui/views/omnibox/brave_omnibox_view_views.h"

#include <chrome/browser/ui/views/location_bar/location_bar_view.cc>

views::View* LocationBarView::GetSearchPromotionButton() const {
  return nullptr;
}

std::vector<views::View*> LocationBarView::GetRightMostTrailingViews() {
  return std::vector<views::View*>();
}

std::vector<views::View*> LocationBarView::GetLeftMostTrailingViews() {
  return std::vector<views::View*>();
}
