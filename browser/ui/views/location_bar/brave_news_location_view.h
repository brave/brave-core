// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_NEWS_LOCATION_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_NEWS_LOCATION_VIEW_H_

#include <string>
#include <vector>

#include "base/scoped_observation.h"
#include "brave/browser/brave_news/brave_news_tab_helper.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"
#include "components/prefs/pref_member.h"
#include "ui/gfx/vector_icon_types.h"
#include "ui/views/view.h"

class Profile;
class BraveNewsBubbleView;

// LocationBar action for Brave News which shows a bubble allowing the user to
// manage feed subscriptions for the current Tab
class BraveNewsLocationView : public PageActionIconView,
                              public BraveNewsTabHelper::PageFeedsObserver,
                              public content::WebContentsObserver {
 public:
  BraveNewsLocationView(
      Profile* profile,
      IconLabelBubbleView::Delegate* icon_label_bubble_delegate,
      PageActionIconView::Delegate* page_action_icon_delegate);
  BraveNewsLocationView(const BraveNewsLocationView&) = delete;
  BraveNewsLocationView& operator=(const BraveNewsLocationView&) = delete;
  ~BraveNewsLocationView() override;

  // PageActionIconView:
  views::BubbleDialogDelegate* GetBubble() const override;
  void UpdateImpl() override;
  std::u16string GetTextForTooltipAndAccessibleName() const override;
  bool ShouldShowLabel() const override;

  // BraveNewsTabHelper::PageFeedsObserver:
  void OnAvailableFeedsChanged(const std::vector<GURL>& feeds) override;

  // views::View:
  void OnThemeChanged() override;

  // content::WebContentsObserver
  void WebContentsDestroyed() override;

 protected:
  // PageActionIconView:
  void OnExecuting(PageActionIconView::ExecuteSource execute_source) override;
  const gfx::VectorIcon& GetVectorIcon() const override;

 private:
  void UpdateIconColor(bool subscribed);
  void OnBubbleClosed();

  base::ScopedObservation<BraveNewsTabHelper,
                          BraveNewsTabHelper::PageFeedsObserver>
      page_feeds_observer_{this};
  BooleanPrefMember should_show_;
  BooleanPrefMember opted_in_;
  BooleanPrefMember news_enabled_;
  raw_ptr<BraveNewsBubbleView> bubble_view_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_NEWS_LOCATION_VIEW_H_
