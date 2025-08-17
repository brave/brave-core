/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_H_

#include <memory>
#include <vector>

#include "base/gtest_prod_util.h"
#include "brave/browser/ui/views/split_view/split_view_separator_delegate.h"
#include "chrome/browser/ui/views/frame/multi_contents_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

namespace views {
class Widget;
}  // namespace views

class SplitViewLocationBar;
class BraveContentsContainerView;

class BraveMultiContentsView : public MultiContentsView,
                               public SplitViewSeparatorDelegate {
  METADATA_HEADER(BraveMultiContentsView, MultiContentsView)

 public:
  static BraveMultiContentsView* From(MultiContentsView* view);

  BraveMultiContentsView(BrowserView* browser_view,
                         std::unique_ptr<MultiContentsViewDelegate> delegate);
  ~BraveMultiContentsView() override;

  void UpdateSecondaryLocationBar();
  void UpdateCornerRadius();

  BraveContentsContainerView* GetActiveContentsContainerView();
  BraveContentsContainerView* GetInactiveContentsContainerView();

 private:
  friend class SplitViewLocationBarBrowserTest;
  friend class SideBySideEnabledBrowserTest;
  friend class SpeedReaderWithSplitViewBrowserTest;
  FRIEND_TEST_ALL_PREFIXES(SideBySideEnabledBrowserTest,
                           BraveMultiContentsViewTest);

  // MultiContentsView:
  void Layout(PassKey) override;

  // SplitViewSeparatorDelegate:
  void OnDoubleClicked() override;

  std::vector<ContentsContainerView*> contents_container_views_for_testing()
      const {
    return contents_container_views_;
  }

  std::unique_ptr<SplitViewLocationBar> secondary_location_bar_;
  std::unique_ptr<views::Widget> secondary_location_bar_widget_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_H_
