/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_help_bubble/shields_help_bubble_controller.h"

#include <memory>

#include "base/check_deref.h"
#include "base/types/to_address.h"
#include "brave/browser/ui/page_info/features.h"
#include "brave/browser/ui/views/brave_actions/brave_shields_action_view.h"
#include "brave/browser/ui/views/brave_actions/brave_shields_toolbar_button.h"
#include "brave/browser/ui/views/brave_help_bubble/brave_help_bubble_host_view.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/interaction/browser_elements_views.h"
#include "chrome/browser/ui/web_applications/app_browser_controller.h"
#include "ui/views/view.h"
#include "ui/views/view_utils.h"

DEFINE_USER_DATA(ShieldsHelpBubbleController);

ShieldsHelpBubbleController::ShieldsHelpBubbleController(
    BrowserWindowInterface& browser)
    : browser_(browser),
      scoped_unowned_user_data_(browser.GetUnownedUserDataHost(), *this) {
  browser.GetTabStripModel()->AddObserver(this);
}

ShieldsHelpBubbleController::~ShieldsHelpBubbleController() = default;

// static
ShieldsHelpBubbleController* ShieldsHelpBubbleController::From(
    BrowserWindowInterface* browser) {
  return Get(browser->GetUnownedUserDataHost());
}

bool ShieldsHelpBubbleController::Show(const std::string& text) {
  // Page Info hosts Shields there, so there is no icon to point to.
  if (page_info::features::IsShowBraveShieldsInPageInfoEnabled()) {
    return false;
  }

  views::View* shields_icon = GetShieldsIcon();
  if (!shields_icon || !shields_icon->GetVisible()) {
    return false;
  }

  // The host view is drawn over the icon, so it is a child of the window's
  // top-level view.
  if (!host_view_) {
    host_view_ = CHECK_DEREF(BrowserView::GetBrowserViewForBrowser(
                                 base::to_address(browser_)))
                     .AddChildView(std::make_unique<BraveHelpBubbleHostView>());
  }
  host_view_->set_text(text);
  host_view_->set_tracked_element(shields_icon);
  return host_view_->Show();
}

bool ShieldsHelpBubbleController::IsShowingForTesting() const {
  return host_view_ && host_view_->IsBubbleOpen();
}

views::View* ShieldsHelpBubbleController::GetShieldsIcon() {
  const bool is_web_app =
      web_app::AppBrowserController::IsWebApp(base::to_address(browser_));
  // Web app windows have both views, which share the identifier.
  for (views::View* view :
       CHECK_DEREF(BrowserElementsViews::From(base::to_address(browser_)))
           .GetAllViews(BraveShieldsActionView::kShieldsActionIcon)) {
    if (is_web_app ? views::IsViewClass<BraveShieldsToolbarButton>(view)
                   : views::IsViewClass<BraveShieldsActionView>(view)) {
      return view;
    }
  }
  return nullptr;
}

void ShieldsHelpBubbleController::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (selection.active_tab_changed() && host_view_ &&
      host_view_->GetVisible()) {
    host_view_->Hide();
  }
}
