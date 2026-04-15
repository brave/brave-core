/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_CONTAINERS_CONTAINERS_BOOKMARK_MENU_MODEL_DELEGATE_H_
#define BRAVE_BROWSER_UI_CONTAINERS_CONTAINERS_BOOKMARK_MENU_MODEL_DELEGATE_H_

#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/containers/containers_menu_model.h"
#include "url/gurl.h"

class Browser;

namespace containers {

// Delegate for `containers::ContainersMenuModel` when opened from the
// bookmark context menu for a single URL bookmark.
class ContainersBookmarkMenuModelDelegate
    : public ContainersMenuModel::Delegate {
 public:
  ContainersBookmarkMenuModelDelegate(Browser* browser,
                                      const GURL& bookmark_url);
  ~ContainersBookmarkMenuModelDelegate() override;

  ContainersBookmarkMenuModelDelegate(
      const ContainersBookmarkMenuModelDelegate&) = delete;
  ContainersBookmarkMenuModelDelegate& operator=(
      const ContainersBookmarkMenuModelDelegate&) = delete;

  void OnContainerSelected(const mojom::ContainerPtr& container) override;
  void OnNoContainerSelected() override;
  base::flat_set<std::string> GetCurrentContainerIds() override;
  Browser* GetBrowserToOpenSettings() override;
  float GetScaleFactor() override;

 private:
  const raw_ptr<Browser> browser_;
  const GURL bookmark_url_;
};

}  // namespace containers

#endif  // BRAVE_BROWSER_UI_CONTAINERS_CONTAINERS_BOOKMARK_MENU_MODEL_DELEGATE_H_
