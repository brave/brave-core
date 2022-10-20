// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_NEWS_LOCATION_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_NEWS_LOCATION_VIEW_H_

#include "ui/views/view.h"

class Profile;
class TabStripModel;

class BraveNewsLocationView : public views::View {
 public:
  BraveNewsLocationView(Profile* profile, TabStripModel* tab_strip_model);
  BraveNewsLocationView(const BraveNewsLocationView&) = delete;
  BraveNewsLocationView& operator=(const BraveNewsLocationView&) = delete;
  ~BraveNewsLocationView() override;

  void Update();

 private:
  views::View* button_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_NEWS_LOCATION_VIEW_H_
