/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/containers/containers_bookmark_menu_model_delegate.h"

#include "base/check.h"
#include "base/check_is_test.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/components/containers/core/common/features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/compositor/compositor.h"
#include "ui/views/widget/widget.h"

namespace containers {

ContainersBookmarkMenuModelDelegate::ContainersBookmarkMenuModelDelegate(
    BrowserWindowInterface* browser,
    const GURL& bookmark_url)
    : browser_(browser), bookmark_url_(bookmark_url) {
  CHECK(base::FeatureList::IsEnabled(features::kContainers));
}

ContainersBookmarkMenuModelDelegate::~ContainersBookmarkMenuModelDelegate() =
    default;

void ContainersBookmarkMenuModelDelegate::OnContainerSelected(
    const mojom::ContainerPtr& container) {
  CHECK(browser_);
  brave::OpenUrlInContainer(base::to_address(browser_), bookmark_url_,
                            container);
}

void ContainersBookmarkMenuModelDelegate::OnNoContainerSelected() {
  CHECK(browser_);
  brave::OpenUrlWithoutContainer(base::to_address(browser_), bookmark_url_);
}

void ContainersBookmarkMenuModelDelegate::OnNewTemporaryContainerSelected() {
  CHECK(browser_);
  brave::CreateTemporaryContainerAndOpenUrl(base::to_address(browser_),
                                            bookmark_url_);
}

base::flat_set<std::string>
ContainersBookmarkMenuModelDelegate::GetCurrentContainerIds() {
  // The bookmark menu is not associated with any tab or link, so there are no
  // current container IDs.
  return {};
}

Browser* ContainersBookmarkMenuModelDelegate::GetBrowserToOpenSettings() {
  CHECK(browser_);
  return browser_->GetBrowserForMigrationOnly();
}

float ContainersBookmarkMenuModelDelegate::GetScaleFactor() {
  if (!browser_) {
    CHECK_IS_TEST();
    return 1.0f;
  }
  auto* browser_view =
      BrowserView::GetBrowserViewForBrowser(base::to_address(browser_));
  if (!browser_view) {
    CHECK_IS_TEST();
    return 1.0f;
  }
  views::Widget* widget = browser_view->GetWidget();
  CHECK(widget);
  ui::Compositor* compositor = widget->GetCompositor();
  CHECK(compositor);
  return compositor->device_scale_factor();
}

}  // namespace containers
