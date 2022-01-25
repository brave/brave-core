/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_STAR_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_STAR_VIEW_H_

#include "chrome/browser/ui/views/location_bar/star_view.h"

class BraveStarView : public StarView {
 public:
  using StarView::StarView;

  BraveStarView(const BraveStarView&) = delete;
  BraveStarView& operator=(const BraveStarView&) = delete;

 protected:
  // views::View:
  void UpdateImpl() override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_STAR_VIEW_H_
