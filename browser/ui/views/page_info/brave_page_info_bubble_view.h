/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PAGE_INFO_BRAVE_PAGE_INFO_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PAGE_INFO_BRAVE_PAGE_INFO_BUBBLE_VIEW_H_

#include <utility>

#include "base/scoped_observation.h"
#include "brave/browser/brave_shields/brave_shields_tab_helper.h"
#include "brave/browser/ui/views/page_info/brave_page_info_tab_switcher.h"
#include "chrome/browser/ui/views/page_info/page_info_bubble_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

// Brave-customized version of Chromium's page info bubble, which displays
// shields, permission, and security information for the current site.
class BravePageInfoBubbleView
    : public PageInfoBubbleView,
      public brave_shields::BraveShieldsTabHelper::Observer {
  METADATA_HEADER(BravePageInfoBubbleView, PageInfoBubbleView)

 public:
  BravePageInfoBubbleView(const BravePageInfoBubbleView&) = delete;
  BravePageInfoBubbleView& operator=(const BravePageInfoBubbleView&) = delete;

  ~BravePageInfoBubbleView() override;

  // PageInfoBubbleView:
  void OpenMainPage(base::OnceClosure initialized_callback) override;
  void AnnouncePageOpened(std::u16string announcement) override;
  void OnWidgetDestroying(views::Widget* widget) override;

  // views::View:
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void ChildPreferredSizeChanged(View* child) override;

  // brave_shields::BraveShieldsTabHelper::Observer:
  void OnResourcesChanged() override;
  void OnShieldsEnabledChanged() override;

 private:
  friend class PageInfoBubbleView;

  template <typename... Args>
  explicit BravePageInfoBubbleView(Args&&... args)
      : PageInfoBubbleView(std::forward<Args>(args)...) {
    InitializeView();
  }

  // Performs view initialization.
  void InitializeView();

  // Applies Brave-specific customizations to the Chromium page info views.
  void CustomizeChromiumViews();

  // Sets the currently active tab.
  void SwitchToTab(BravePageInfoTabSwitcher::Tab tab);

  // Updates content visibility based on the current tab.
  void UpdateContentVisibilityForCurrentTab();

  // Returns a value indicating whether the specified child view was created by
  // the parent class and belongs in the "Site settings" tab.
  bool IsSiteSettingsChildView(views::View* view) const;

  // Returns a value indicating whether a subpage is currently active in the
  // site settings tab.
  bool IsSiteSettingsSubpageActive() const;

  // Returns a value indicating whether Shields is enabled for the current tab.
  bool IsShieldsEnabledForWebContents();

  // UI components.
  raw_ptr<BravePageInfoTabSwitcher> tab_switcher_ = nullptr;

  base::ScopedObservation<brave_shields::BraveShieldsTabHelper,
                          brave_shields::BraveShieldsTabHelper::Observer>
      shields_observation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PAGE_INFO_BRAVE_PAGE_INFO_BUBBLE_VIEW_H_
