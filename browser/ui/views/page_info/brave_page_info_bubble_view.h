/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PAGE_INFO_BRAVE_PAGE_INFO_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PAGE_INFO_BRAVE_PAGE_INFO_BUBBLE_VIEW_H_

#include <utility>

#include "base/scoped_observation.h"
#include "brave/browser/brave_shields/brave_shields_tab_helper.h"
#include "chrome/browser/ui/views/page_info/page_info_bubble_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

class BraveShieldsPageInfoView;

namespace gfx {
struct VectorIcon;
}

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
  void Layout(PassKey) override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void ChildPreferredSizeChanged(View* child) override;

  // WebContentsObserver:
  void PrimaryPageChanged(content::Page& page) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  // brave_shields::BraveShieldsTabHelper::Observer:
  void OnResourcesChanged() override;
  void OnShieldsEnabledChanged() override;

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
  void UpdateTabButtons();
  void UpdateTabButton(Tab tab);

  // Returns the tab switcher button for the specified tab.
  views::LabelButton* GetButtonForTab(Tab tab);

  // Updates the position of the tab indicator bar under the active button.
  void UpdateTabIndicator();

  // Returns a value indicating whether the specified child view was created by
  // the parent class and belongs in the "Site settings" tab.
  bool IsSiteSettingsChildView(views::View* view) const;

  // Returns a value indicating whether a subpage is currently active in the
  // site settings tab.
  bool IsSiteSettingsSubpageActive() const;

  // Returns a value indicating whether Shields is enabled for the current tab.
  bool IsShieldsEnabledForWebContents();

  // Returns the appropriate text and icon for the specified tab.
  int GetTabButtonText(Tab tab) const;
  const gfx::VectorIcon& GetTabButtonIcon(Tab tab);

  // UI components.
  raw_ptr<views::View> tab_switcher_ = nullptr;
  raw_ptr<views::LabelButton> shields_button_ = nullptr;
  raw_ptr<views::LabelButton> site_settings_button_ = nullptr;
  raw_ptr<views::View> tab_indicator_ = nullptr;
  raw_ptr<BraveShieldsPageInfoView> shields_page_view_ = nullptr;

  // The currently active tab.
  Tab current_tab_ = Tab::kSiteSettings;

  // Scoped observation of the Shields tab helper.
  base::ScopedObservation<brave_shields::BraveShieldsTabHelper,
                          brave_shields::BraveShieldsTabHelper::Observer>
      shields_observation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PAGE_INFO_BRAVE_PAGE_INFO_BUBBLE_VIEW_H_
