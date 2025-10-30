/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/page_info/brave_page_info_bubble_view.h"

#include "brave/browser/ui/page_info/features.h"
#include "chrome/browser/ui/views/page_info/page_info_view_factory.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/view_class_properties.h"

using Tab = BravePageInfoTabSwitcher::Tab;

BravePageInfoBubbleView::~BravePageInfoBubbleView() = default;

void BravePageInfoBubbleView::OpenMainPage(
    base::OnceClosure initialized_callback) {
  PageInfoBubbleView::OpenMainPage(std::move(initialized_callback));
  CustomizeChromiumViews();
  SizeToContents();
}

void BravePageInfoBubbleView::AnnouncePageOpened(std::u16string announcement) {
  // This method is called after any PageInfo subpage is opened and allows us to
  // customize child views added by the superclass.
  PageInfoBubbleView::AnnouncePageOpened(std::move(announcement));
  CustomizeChromiumViews();
  SizeToContents();

  // When a subpage is opened programmatically (e.g. when the page info bubble
  // is opened directly to a subpage), ensure that the Site Settings tab is
  // active.
  if (page_info::features::IsShowBraveShieldsInPageInfoEnabled()) {
    SwitchToTab(Tab::kSiteSettings);
  }
}

void BravePageInfoBubbleView::OnWidgetDestroying(views::Widget* widget) {
  shields_observation_.Reset();
  PageInfoBubbleView::OnWidgetDestroying(widget);
}

gfx::Size BravePageInfoBubbleView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  gfx::Size size = PageInfoBubbleView::CalculatePreferredSize(available_size);

  if (page_info::features::IsShowBraveShieldsInPageInfoEnabled()) {
    // This bubble needs to be larger than the parent class in order to show the
    // full tab switcher and Shields content.
    constexpr int kMinBubbleWidth = 388;
    size.set_width(std::max(size.width(), kMinBubbleWidth));
  }

  return size;
}

void BravePageInfoBubbleView::ChildPreferredSizeChanged(View* child) {
  // When child preferred sizes change (e.g. when the Shields page view is auto
  // resized), we need to make sure that layout caches are dropped before
  // calculating the size of the bubble.
  InvalidateLayout();
  SizeToContents();
}

void BravePageInfoBubbleView::OnResourcesChanged() {
  CHECK(tab_switcher_);
  tab_switcher_->SetShieldsEnabled(IsShieldsEnabledForWebContents());
}

void BravePageInfoBubbleView::OnShieldsEnabledChanged() {
  CHECK(tab_switcher_);
  tab_switcher_->SetShieldsEnabled(IsShieldsEnabledForWebContents());
}

void BravePageInfoBubbleView::InitializeView() {
  if (!page_info::features::IsShowBraveShieldsInPageInfoEnabled()) {
    return;
  }

  // Obtain a reference to the Shields tab helper and observe it.  This will
  // allow us to customize the view based on the current Shields status.
  auto* shields_tab_helper =
      brave_shields::BraveShieldsTabHelper::FromWebContents(web_contents());
  CHECK(shields_tab_helper);
  shields_observation_.Observe(shields_tab_helper);

  CustomizeChromiumViews();

  // Remove the top margin set by the parent class.
  set_margins(gfx::Insets());

  // Add the tab switcher at the top of the bubble.
  auto tab_switcher =
      std::make_unique<BravePageInfoTabSwitcher>(base::BindRepeating(
          &BravePageInfoBubbleView::SwitchToTab, base::Unretained(this)));
  tab_switcher_ = AddChildViewAt(std::move(tab_switcher), 0);

  // If the PageInfo bubble was not opened directly to a subpage, then show the
  // Shields tab first.
  tab_switcher_->SetCurrentTab(
      IsSiteSettingsSubpageActive() ? Tab::kSiteSettings : Tab::kShields);
  OnShieldsEnabledChanged();

  UpdateContentVisibilityForCurrentTab();
  SizeToContents();
}

void BravePageInfoBubbleView::CustomizeChromiumViews() {
  if (!page_info::features::IsShowBraveShieldsInPageInfoEnabled()) {
    return;
  }

  // Hide the close button in the page header for the main page or any subpage.
  auto* close_button =
      GetViewByID(PageInfoViewFactory::VIEW_ID_PAGE_INFO_CLOSE_BUTTON);
  if (close_button) {
    close_button->SetVisible(false);
  }

  // Find the first site settings content child and set its top margin.
  for (views::View* child : children()) {
    if (IsSiteSettingsChildView(child)) {
      const int top_margin = IsSiteSettingsSubpageActive() ? 16 : 8;
      child->SetProperty(views::kMarginsKey, gfx::Insets().set_top(top_margin));
      break;
    }
  }
}

void BravePageInfoBubbleView::SwitchToTab(Tab tab) {
  CHECK(tab_switcher_);
  tab_switcher_->SetCurrentTab(tab);
  UpdateContentVisibilityForCurrentTab();
  SizeToContents();
}

void BravePageInfoBubbleView::UpdateContentVisibilityForCurrentTab() {
  CHECK(tab_switcher_);

  // Show/hide Chromium page info content.
  Tab current_tab = tab_switcher_->GetCurrentTab();
  for (views::View* child : children()) {
    if (IsSiteSettingsChildView(child)) {
      child->SetVisible(current_tab == Tab::kSiteSettings);
    }
  }
}

bool BravePageInfoBubbleView::IsSiteSettingsChildView(views::View* view) const {
  return view->parent() == this && view != tab_switcher_;
}

bool BravePageInfoBubbleView::IsSiteSettingsSubpageActive() const {
  // If a back button exists in the view tree, then we known that the view is
  // displaying a subpage.
  auto* back_button =
      GetViewByID(PageInfoViewFactory::VIEW_ID_PAGE_INFO_BACK_BUTTON);
  return back_button;
}

bool BravePageInfoBubbleView::IsShieldsEnabledForWebContents() {
  auto* tab_helper = shields_observation_.GetSource();
  return tab_helper && tab_helper->GetBraveShieldsEnabled();
}

BEGIN_METADATA(BravePageInfoBubbleView)
END_METADATA
