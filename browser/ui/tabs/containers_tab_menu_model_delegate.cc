/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/containers_tab_menu_model_delegate.h"

#include "brave/browser/ui/browser_commands.h"
#include "brave/components/containers/core/browser/storage_partition_constants.h"
#include "brave/components/containers/core/common/features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "ui/compositor/compositor.h"
#include "ui/views/widget/widget.h"

namespace brave {

ContainersTabMenuModelDelegate::ContainersTabMenuModelDelegate(
    BrowserWindowInterface* browser_window,
    const std::vector<tabs::TabHandle>& selected_tabs)
    : browser_window_(browser_window), selected_tabs_(selected_tabs) {
  CHECK(base::FeatureList::IsEnabled(containers::features::kContainers));
}

ContainersTabMenuModelDelegate::~ContainersTabMenuModelDelegate() = default;

void ContainersTabMenuModelDelegate::OnContainerSelected(
    const containers::mojom::ContainerPtr& container) {
  for (auto tab_handle : selected_tabs_) {
    brave::OpenTabUrlInContainer(browser_window_.get(), tab_handle, container);
  }
}

base::flat_set<std::string>
ContainersTabMenuModelDelegate::GetCurrentContainerIds() {
  base::flat_set<std::string> container_ids;
  for (auto tab_handle : selected_tabs_) {
    auto* tab = tab_handle.Get();
    if (!tab) {
      continue;
    }

    auto* contents = tab->GetContents();
    if (!contents) {
      continue;
    }

    auto storage_partition_config =
        contents->GetSiteInstance()->GetStoragePartitionConfig();
    if (storage_partition_config.partition_domain() !=
        containers::kContainersStoragePartitionDomain) {
      continue;
    }

    if (storage_partition_config.partition_name().empty()) {
      continue;
    }

    container_ids.insert(storage_partition_config.partition_name());
  }

  return container_ids;
}

Browser* ContainersTabMenuModelDelegate::GetBrowserToOpenSettings() {
  return browser_window_->GetBrowserForMigrationOnly();
}

float ContainersTabMenuModelDelegate::GetScaleFactor() {
  // Get the device scale factor from the browser window's widget.
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser_window_);
  CHECK(browser_view);
  auto* widget = browser_view->GetWidget();
  CHECK(widget);
  auto* compositor = widget->GetCompositor();
  CHECK(compositor);
  return compositor->device_scale_factor();
}

}  // namespace brave
