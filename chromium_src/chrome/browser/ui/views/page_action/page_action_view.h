// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_PAGE_ACTION_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_PAGE_ACTION_VIEW_H_

#include <chrome/browser/ui/views/page_action/page_action_view.h>  // IWYU pragma: export

// Add methods to override the IconLabelBubbleView methods.
// Also add a friend class for testing.
namespace page_actions {
class PageActionView : public chromium_impl::PageActionView {
  METADATA_HEADER(PageActionView, chromium_impl::PageActionView)

 public:
  using chromium_impl::PageActionView::PageActionView;
  ~PageActionView() override;

  // This callback is proxy of the
  // chromium_impl::PageActionView::AddChipVisibilityChangedCallback. As the
  // upstream's callback takes chromium_impl::PageActionView* as an argument, we
  // need to convert it to PageActionView* by wrapping it in a lambda.
  base::CallbackListSubscription AddChipVisibilityChangedCallback(
      base::RepeatingCallback<void(PageActionView*)> callback);

  // chromium_impl::PageActionView:
  views::ProposedLayout CalculateProposedLayout(
      const views::SizeBounds& size_bounds) const override;
  gfx::Size GetSizeForLabelWidth(int label_width) const override;
  bool ShouldShowLabel() const override;
  SkColor GetBackgroundColor() const override;
  SkColor GetForegroundColor() const override;
  std::optional<int> GetOverrideHeight() const;
  void OnPageActionModelVisualRefresh(PageActionModelInterface* model);
  void OnPageActionModelChanged(const PageActionModelInterface& model) override;
  gfx::Size GetMinimumSize() const override;
  bool ShouldAlwaysShowLabel() const override;
  void OnNewActiveController(PageActionController* controller) override;
  void UpdateBorder() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(PageActionViewTest,
                           AlwaysShowsLabelEnsuresLabelWidth);
  FRIEND_TEST_ALL_PREFIXES(PageActionViewTest, UseTonalColorsWhenExpanded);
  FRIEND_TEST_ALL_PREFIXES(PageActionViewTest,
                           DefaultBackgroundColorIsTransparent);
  FRIEND_TEST_ALL_PREFIXES(PageActionViewTest,
                           OverrideBackgroundColorReturnsModelValue);
};

}  // namespace page_actions

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_PAGE_ACTION_VIEW_H_
