/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_LOCATION_BAR_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_LOCATION_BAR_VIEW_H_

#include "chrome/browser/ui/views/location_bar/location_bar_view.h"

class BraveActionsContainer;
class BraveActionsContainerTest;
class RewardsBrowserTest;
class SkPath;

// The purposes of this subclass are to:
// - Add the BraveActionsContainer to the location bar
class BraveLocationBarView : public LocationBarView {
 public:
  using LocationBarView::LocationBarView;
  void Init() override;
  void Layout() override;
  void Update(content::WebContents* contents) override;
  void OnChanged() override;
  BraveActionsContainer* GetBraveActionsContainer() { return brave_actions_; }

  // views::View:
  gfx::Size CalculatePreferredSize() const override;
  void OnThemeChanged() override;
  void ChildPreferredSizeChanged(views::View* child) override;

  int GetBorderRadius() const override;

  SkPath GetFocusRingHighlightPath() const;
  ContentSettingImageView* GetContentSettingsImageViewForTesting(size_t idx);

 private:
  friend class ::BraveActionsContainerTest;
  friend class ::RewardsBrowserTest;
  BraveActionsContainer* brave_actions_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(BraveLocationBarView);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_LOCATION_BAR_VIEW_H_
