// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_NEWS_BRAVE_NEWS_ACTION_ICON_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_NEWS_BRAVE_NEWS_ACTION_ICON_VIEW_H_

#include <string>
#include <vector>

#include "base/scoped_observation.h"
#include "brave/browser/brave_news/brave_news_tab_helper.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"
#include "components/prefs/pref_member.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/gfx/vector_icon_types.h"
#include "ui/views/view.h"

class Profile;

namespace brave_news {
class BraveNewsBubbleController;
}

// LocationBar action for Brave News which shows a bubble allowing the user to
// manage feed subscriptions for the current Tab
class BraveNewsActionIconView : public PageActionIconView,
                                public BraveNewsTabHelper::PageFeedsObserver,
                                public content::WebContentsObserver {
  METADATA_HEADER(BraveNewsActionIconView, PageActionIconView)
 public:
  BraveNewsActionIconView(
      Profile* profile,
      IconLabelBubbleView::Delegate* icon_label_bubble_delegate,
      PageActionIconView::Delegate* page_action_icon_delegate);
  BraveNewsActionIconView(const BraveNewsActionIconView&) = delete;
  BraveNewsActionIconView& operator=(const BraveNewsActionIconView&) = delete;
  ~BraveNewsActionIconView() override;

  base::WeakPtr<BraveNewsActionIconView> AsWeakPtr();

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
  brave_news::BraveNewsBubbleController* GetController() const;

  // PageActionIconView:
  void OnExecuting(PageActionIconView::ExecuteSource execute_source) override;
  const gfx::VectorIcon& GetVectorIcon() const override;

 private:
  void UpdateIconColor(bool subscribed);
  void ShowBraveNewsBubble();

  base::ScopedObservation<BraveNewsTabHelper,
                          BraveNewsTabHelper::PageFeedsObserver>
      page_feeds_observer_{this};
  BooleanPrefMember should_show_;
  BooleanPrefMember opted_in_;
  BooleanPrefMember news_enabled_;

  base::WeakPtrFactory<BraveNewsActionIconView> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_NEWS_BRAVE_NEWS_ACTION_ICON_VIEW_H_
