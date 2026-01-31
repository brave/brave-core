/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_CONTAINERS_TAB_MENU_MODEL_DELEGATE_H_
#define BRAVE_BROWSER_UI_TABS_CONTAINERS_TAB_MENU_MODEL_DELEGATE_H_

#include <vector>

#include "brave/browser/ui/containers/containers_menu_model.h"
#include "components/tabs/public/tab_interface.h"

class BrowserWindowInterface;

namespace brave {

class ContainersTabMenuModelDelegate
    : public containers::ContainersMenuModel::Delegate {
 public:
  ContainersTabMenuModelDelegate(
      BrowserWindowInterface* browser_window,
      const std::vector<tabs::TabHandle>& selected_tabs);
  ~ContainersTabMenuModelDelegate() override;

  void OnContainerSelected(
      const containers::mojom::ContainerPtr& container) override;

  base::flat_set<std::string> GetCurrentContainerIds() override;

  Browser* GetBrowserToOpenSettings() override;

  float GetScaleFactor() override;

 private:
  raw_ptr<BrowserWindowInterface> browser_window_ = nullptr;
  std::vector<tabs::TabHandle> selected_tabs_;
};

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_TABS_CONTAINERS_TAB_MENU_MODEL_DELEGATE_H_
