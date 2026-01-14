/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_browser_tab_menu_model_delegate.h"

#include "base/notimplemented.h"
#include "base/notreached.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/compositor/compositor.h"
#include "ui/views/widget/widget.h"

namespace brave {

BraveBrowserTabMenuModelDelegate::BraveBrowserTabMenuModelDelegate(
    SessionID session_id,
    const Profile* profile,
    const web_app::AppBrowserController* app_controller,
    tab_groups::TabGroupSyncService* tgss,
    BrowserWindowInterface* browser_window)
    : chrome::BrowserTabMenuModelDelegate(session_id,
                                          profile,
                                          app_controller,
                                          tgss),
      browser_window_(browser_window) {}

BraveBrowserTabMenuModelDelegate::~BraveBrowserTabMenuModelDelegate() = default;

bool BraveBrowserTabMenuModelDelegate::ShouldShowBraveVerticalTab() {
  // TODO(https://github.com/brave/brave-browser/issues/51112): Avoid
  // GetBrowserForMigrationOnly().
  return tabs::utils::ShouldShowBraveVerticalTabs(
      browser_window_->GetBrowserForMigrationOnly());
}

containers::ContainersMenuModelDelegate*
BraveBrowserTabMenuModelDelegate::GetContainersMenuModelDelegate() {
#if BUILDFLAG(ENABLE_CONTAINERS)
  return this;
#else
  NOTREACHED();
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
}

#if BUILDFLAG(ENABLE_CONTAINERS)
void BraveBrowserTabMenuModelDelegate::OnContainerSelected(
    const containers::mojom::ContainerPtr& container) {
  // TODO(https://github.com/brave/brave-browser/issues/46352) Implement when
  // the containers feature is ready.
  NOTIMPLEMENTED();
}

base::flat_set<std::string>
BraveBrowserTabMenuModelDelegate::GetCurrentContainerIds() {
  // TODO(https://github.com/brave/brave-browser/issues/46352) Fill the set with
  // container ids of tabs selected.
  NOTIMPLEMENTED();
  return {};
}

Browser* BraveBrowserTabMenuModelDelegate::GetBrowserToOpenSettings() {
  return browser_window_->GetBrowserForMigrationOnly();
}

float BraveBrowserTabMenuModelDelegate::GetScaleFactor() {
  // Get the device scale factor from the browser window's widget.
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser_window_);
  CHECK(browser_view);
  auto* widget = browser_view->GetWidget();
  CHECK(widget);
  auto* compositor = widget->GetCompositor();
  CHECK(compositor);
  return compositor->device_scale_factor();
}
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

}  // namespace brave
