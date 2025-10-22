/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_H_

#include <memory>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/views/frame/multi_contents_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

namespace sidebar {
FORWARD_DECLARE_TEST(SidebarBrowserWithWebPanelTest, WebPanelTest);
}  // namespace sidebar

class BraveContentsContainerView;

class BraveMultiContentsView : public MultiContentsView {
  METADATA_HEADER(BraveMultiContentsView, MultiContentsView)

 public:
  static BraveMultiContentsView* From(MultiContentsView* view);

  BraveMultiContentsView(BrowserView* browser_view,
                         std::unique_ptr<MultiContentsViewDelegate> delegate);
  ~BraveMultiContentsView() override;

  void UpdateCornerRadius();
  void UseContentsContainerViewForWebPanel();
  void SetWebPanelVisible(bool visible);
  bool IsWebPanelVisible() const;

  void SetWebPanelWidth(int width);
  void SetWebPanelOnLeft(bool left);

  BraveContentsContainerView* GetActiveContentsContainerView();
  BraveContentsContainerView* GetInactiveContentsContainerView();

 private:
  friend class SideBySideEnabledBrowserTest;
  friend class SpeedReaderWithSplitViewBrowserTest;
  FRIEND_TEST_ALL_PREFIXES(SideBySideEnabledBrowserTest,
                           BraveMultiContentsViewTest);
  FRIEND_TEST_ALL_PREFIXES(sidebar::SidebarBrowserWithWebPanelTest,
                           WebPanelTest);

  // MultiContentsView:
  void Layout(PassKey) override;
  views::ProposedLayout CalculateProposedLayout(
      const views::SizeBounds& size_bounds) const override;
  void ResetResizeArea() override;

  int GetWebPanelWidth() const;

  std::vector<ContentsContainerView*> contents_container_views_for_testing()
      const {
    return contents_container_views_;
  }

  int web_panel_width_ = 0;
  bool web_panel_on_left_ = false;
  raw_ptr<BraveContentsContainerView> contents_container_view_for_web_panel_ =
      nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_H_
