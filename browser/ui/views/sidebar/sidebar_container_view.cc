/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "brave/browser/ui/views/sidebar/sidebar_container_view.h"

#include "brave/browser/themes/theme_properties.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/sidebar/sidebar_model_data.h"
#include "brave/browser/ui/views/sidebar/sidebar_control_view.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "ui/base/theme_provider.h"
#include "ui/views/border.h"
#include "ui/views/controls/webview/webview.h"
#include "url/gurl.h"

SidebarContainerView::SidebarContainerView(BraveBrowser* browser)
    : browser_(browser) {}

SidebarContainerView::~SidebarContainerView() = default;

void SidebarContainerView::Init() {
  initialized_ = true;

  sidebar_model_ = browser_->sidebar_controller()->model();
  observed_.Add(sidebar_model_);

  AddChildViews();
  UpdateChildViewVisibility();
}

void SidebarContainerView::ShowSidebar(bool show) {
  sidebar_control_view_->SetVisible(show);

  InvalidateLayout();
}

void SidebarContainerView::UpdateSidebar() {
  sidebar_control_view_->Update();
}

void SidebarContainerView::UpdateBackgroundAndBorder() {
  if (const ui::ThemeProvider* theme_provider = GetThemeProvider()) {
    constexpr int kBorderThickness = 1;
    SetBorder(views::CreateSolidSidedBorder(
        0, 0, 0, kBorderThickness,
        theme_provider->GetColor(BraveThemeProperties::COLOR_SIDEBAR_BORDER)));
  }
}

void SidebarContainerView::AddChildViews() {
  sidebar_control_view_ =
      AddChildView(std::make_unique<SidebarControlView>(browser_));
  sidebar_panel_view_ =
      AddChildView(std::make_unique<views::WebView>(browser_->profile()));
  // |sidebar_panel_view_| will be visible when user opens sidebar panel.
  sidebar_panel_view_->SetVisible(false);
}

void SidebarContainerView::UpdateChildViewVisibility() {}

void SidebarContainerView::Layout() {
  if (!initialized_)
    return View::Layout();

  const int control_view_preferred_width =
      sidebar_control_view_->GetPreferredSize().width();
  sidebar_control_view_->SetBounds(0, 0, control_view_preferred_width,
                                   height());
  if (sidebar_panel_view_->GetVisible()) {
    sidebar_panel_view_->SetBounds(
        control_view_preferred_width, 0,
        width() - control_view_preferred_width - GetInsets().width(), height());
  }
}

gfx::Size SidebarContainerView::CalculatePreferredSize() const {
  if (!initialized_ || !sidebar_control_view_->GetVisible())
    return View::CalculatePreferredSize();

  const int control_view_preferred_width =
      sidebar_control_view_->GetPreferredSize().width();

  constexpr int kSidebarContainerViewFullWidth = 380;
  return {sidebar_panel_view_->GetVisible() ? kSidebarContainerViewFullWidth
                                            : control_view_preferred_width,
          0};
}

void SidebarContainerView::OnThemeChanged() {
  View::OnThemeChanged();

  UpdateBackgroundAndBorder();
}

void SidebarContainerView::OnActiveIndexChanged(int old_index, int new_index) {
  if (new_index == -1) {
    sidebar_panel_view_->SetVisible(false);
  } else {
    const auto item = sidebar_model_->GetAllSidebarItems()[new_index];
    if (item.open_in_panel) {
      sidebar_panel_view_->SetWebContents(
          sidebar_model_->GetWebContentsAt(new_index));
      sidebar_panel_view_->SetVisible(true);
    } else {
      sidebar_panel_view_->SetVisible(false);
    }
  }

  InvalidateLayout();
}
