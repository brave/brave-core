// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_NEWS_BRAVE_NEWS_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_NEWS_BRAVE_NEWS_BUBBLE_VIEW_H_

#include "base/memory/raw_ptr.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

class BraveNewsFeedsContainerView;
namespace brave_news {
class BraveNewsBubbleController;
}

class BraveNewsBubbleView : public views::BubbleDialogDelegateView {
  METADATA_HEADER(BraveNewsBubbleView, views::BubbleDialogDelegateView)
 public:
  static base::WeakPtr<views::Widget> Show(views::View* anchor,
                                           content::WebContents* contents);

  explicit BraveNewsBubbleView(views::View* action_view,
                               content::WebContents* contents);
  BraveNewsBubbleView(const BraveNewsBubbleView&) = delete;
  BraveNewsBubbleView& operator=(const BraveNewsBubbleView&) = delete;
  ~BraveNewsBubbleView() override;

  void OpenManageFeeds();

  // views::BubbleDialogDelegateView:
  void OnWidgetDestroyed(views::Widget*) override;
  void OnThemeChanged() override;

 private:
  raw_ptr<content::WebContents> contents_ = nullptr;
  raw_ptr<views::Label> title_label_ = nullptr;
  raw_ptr<views::Label> subtitle_label_ = nullptr;
  raw_ptr<BraveNewsFeedsContainerView> feeds_container_ = nullptr;

  base::WeakPtr<brave_news::BraveNewsBubbleController> controller_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_NEWS_BRAVE_NEWS_BUBBLE_VIEW_H_
