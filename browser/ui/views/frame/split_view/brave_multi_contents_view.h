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
#include "ui/gfx/geometry/rounded_corners_f.h"

namespace sidebar {
FORWARD_DECLARE_TEST(SidebarBrowserWithWebPanelTest, WebPanelTest);
}  // namespace sidebar

class BraveContentsContainerView;

namespace views {
class WebView;
}  // namespace views

class BraveMultiContentsView : public MultiContentsView {
  METADATA_HEADER(BraveMultiContentsView, MultiContentsView)

 public:
  static BraveMultiContentsView* From(MultiContentsView* view);

  BraveMultiContentsView(BrowserView* browser_view,
                         std::unique_ptr<MultiContentsViewDelegate> delegate);
  ~BraveMultiContentsView() override;

  void UseContentsContainerViewForWebPanel();
  void SetWebPanelContents(content::WebContents* web_contents);
  bool IsWebPanelVisible() const;

  void SetWebPanelWidth(int width);
  void SetWebPanelOnLeft(bool left);

  void set_web_panel_active(bool active) { is_web_panel_active_ = active; }
  bool is_web_panel_active_for_testing() const { return is_web_panel_active_; }
  views::View* GetWebPanelContentsViewForTesting() const;

  // Notifies the multi-contents view that the browser's active content domain
  // display state has changed.
  void OnShowActiveContentsDomainChanged();

  // Updates the contents area corner radii for the hosted contents areas, based
  // on the specified corner radii for the multi-contents view as a whole.
  void UpdateContentsCornerRadii(const gfx::RoundedCornersF& corner_radii);

  // MultiContentsView:
  // Give web panel's ContentsContainerView/ContentsWebView if
  // |is_web_panel_active_| is true.
  ContentsContainerView* GetActiveContentsContainerView() const override;
  ContentsWebView* GetActiveContentsView() const override;
  ContentsContainerView* GetContentsContainerViewFor(
      content::WebContents* web_contents) const override;

 private:
  friend class SplitViewBrowserTest;
  friend class SpeedReaderWithSplitViewBrowserTest;
  FRIEND_TEST_ALL_PREFIXES(SplitViewBrowserTest, BraveMultiContentsViewTest);
  FRIEND_TEST_ALL_PREFIXES(sidebar::SidebarBrowserWithWebPanelTest,
                           WebPanelTest);

  // MultiContentsView:
  views::ProposedLayout CalculateProposedLayout(
      const views::SizeBounds& size_bounds) const override;
  void ResetResizeArea() override;
  void UpdateContentsBorderAndOverlay() override;
  void OnWebContentsFocused(views::WebView* web_view) override;
  void ExecuteOnEachVisibleContentsView(
      base::RepeatingCallback<void(ContentsWebView*)> callback) override;

  int GetWebPanelWidth() const;

  std::vector<ContentsContainerView*> contents_container_views_for_testing()
      const {
    return std::vector<ContentsContainerView*>(
        contents_container_views_.begin(), contents_container_views_.end());
  }

  int web_panel_width_ = 0;
  bool web_panel_on_left_ = false;

  // true when web panel is activated.
  bool is_web_panel_active_ = false;
  raw_ptr<BraveContentsContainerView> contents_container_view_for_web_panel_ =
      nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_SPLIT_VIEW_BRAVE_MULTI_CONTENTS_VIEW_H_
