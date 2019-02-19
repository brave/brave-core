/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_LOCATION_ICON_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_LOCATION_ICON_VIEW_H_

#include "chrome/browser/ui/views/location_bar/location_icon_view.h"

class BraveLocationIconView : public LocationIconView {
 public:
  using LocationIconView::LocationIconView;
  ~BraveLocationIconView() override = default;

 private:
  // LocationIconView overrides:
  base::string16 GetText() const override;
  bool ShouldShowText() const override;

  DISALLOW_COPY_AND_ASSIGN(BraveLocationIconView);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_LOCATION_ICON_VIEW_H_
