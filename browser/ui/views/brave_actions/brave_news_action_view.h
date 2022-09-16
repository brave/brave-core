// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_NEWS_ACTION_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_NEWS_ACTION_VIEW_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "brave/browser/brave_news/brave_news_tab_helper.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "components/prefs/pref_member.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/controls/button/label_button.h"

class Profile;
class TabStripModel;

class BraveNewsActionView : public views::LabelButton,
                            public TabStripModelObserver,
                            public BraveNewsTabHelper::PageFeedsObserver {
 public:
  BraveNewsActionView(Profile* profile, TabStripModel* tab_strip);
  ~BraveNewsActionView() override;
  BraveNewsActionView(const BraveNewsActionView&) = delete;
  BraveNewsActionView& operator=(const BraveNewsActionView&) = delete;

  void Init();
  void Update();

  // views::LabelButton:
  std::unique_ptr<views::LabelButtonBorder> CreateDefaultBorder()
      const override;
  std::u16string GetTooltipText(const gfx::Point& p) const override;
  SkPath GetHighlightPath() const;

  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  // BraveNewsTabHelper::PageFeedsObserver:
  void OnAvailableFeedsChanged(
      const std::vector<BraveNewsTabHelper::FeedDetails>& feeds) override;

 private:
  void ButtonPressed();

  BooleanPrefMember should_show_;
  BooleanPrefMember news_enabled_;

  base::raw_ptr<Profile> profile_;
  base::raw_ptr<TabStripModel> tab_strip_;
  base::WeakPtr<views::Widget> bubble_widget_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ACTIONS_BRAVE_NEWS_ACTION_VIEW_H_
