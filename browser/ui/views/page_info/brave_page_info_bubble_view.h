/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PAGE_INFO_BRAVE_PAGE_INFO_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PAGE_INFO_BRAVE_PAGE_INFO_BUBBLE_VIEW_H_

#include <utility>

#include "chrome/browser/ui/views/page_info/page_info_bubble_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

// Brave-customized version of Chromium's page info bubble, which displays
// permission and security information for the current site.
class BravePageInfoBubbleView : public PageInfoBubbleView {
  METADATA_HEADER(BravePageInfoBubbleView, PageInfoBubbleView)

 public:
  BravePageInfoBubbleView(const BravePageInfoBubbleView&) = delete;
  BravePageInfoBubbleView& operator=(const BravePageInfoBubbleView&) = delete;

  ~BravePageInfoBubbleView() override;

  // PageInfoBubbleView:
  void OpenMainPage(base::OnceClosure initialized_callback) override;
  void AnnouncePageOpened(std::u16string announcement) override;
  void Layout(PassKey) override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;

  enum class Tab {
    kSiteSettings,
    kShields,
  };

 private:
  friend class PageInfoBubbleView;

  template <typename... Args>
  explicit BravePageInfoBubbleView(Args&&... args)
      : PageInfoBubbleView(std::forward<Args>(args)...) {
    InitializeView();
  }

  void InitializeView();

  // Applies Brave-specific customizations to the Chromium page info views.
  void CustomizeChromiumViews();

  // Creates and returns the tab switcher view which allows switching between
  // the normal page info view and Brave Shields.
  std::unique_ptr<views::View> CreateTabSwitcher();

  // Creates a tab switcher button for the specified tab.
  std::unique_ptr<views::LabelButton> CreateTabButton(Tab tab);

  // Switches to the specified tab.
  void SwitchToTab(Tab tab);

  // Updates content visibility based on the current tab.
  void UpdateContentVisibilityForCurrentTab();

  // Updates the visual state of the tab buttons to reflect the current tab.
  void UpdateAllTabButtonStyles();
  void UpdateTabButtonStyles(Tab tab);

  // Returns the tab switcher button for the specified tab.
  views::LabelButton* GetButtonForTab(Tab tab);

  // Updates the position of the tab indicator bar under the active button.
  void UpdateTabIndicator();

  // Returns a value indicating whether the specified child view was created by
  // the parent class and belongs in the "Site settings" tab.
  bool IsSiteSettingsChildView(views::View* view) const;

  // UI components.
  raw_ptr<views::View> tab_switcher_ = nullptr;
  raw_ptr<views::LabelButton> shields_button_ = nullptr;
  raw_ptr<views::LabelButton> site_settings_button_ = nullptr;
  raw_ptr<views::View> tab_indicator_ = nullptr;
  raw_ptr<views::View> shields_content_view_ = nullptr;

  // The currently active tab.
  Tab current_tab_ = Tab::kSiteSettings;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PAGE_INFO_BRAVE_PAGE_INFO_BUBBLE_VIEW_H_
