// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_CONTAINERS_CONTAINERS_MENU_MODEL_H_
#define BRAVE_BROWSER_UI_CONTAINERS_CONTAINERS_MENU_MODEL_H_

#include <limits>
#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "brave/browser/ui/containers/container_model.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "ui/menus/simple_menu_model.h"

class Browser;
class PrefService;

namespace containers {

namespace test {
class ContainersMenuModelTestApi;
}  // namespace test

class ContainersMenuModelDelegate {
 public:
  ContainersMenuModelDelegate() = default;
  ContainersMenuModelDelegate(const ContainersMenuModelDelegate&) = delete;
  ContainersMenuModelDelegate& operator=(const ContainersMenuModelDelegate&) =
      delete;
  virtual ~ContainersMenuModelDelegate() = default;

  virtual void OnContainerSelected(const mojom::ContainerPtr& container) = 0;
  virtual base::flat_set<std::string> GetCurrentContainerIds() = 0;
  virtual Browser* GetBrowserToOpenSettings() = 0;
  virtual float GetScaleFactor() = 0;
};

// A menu model that represents a list of Containers. This menu can be used in
// various UI components, such as renderer context menus, tab context menus,
// etc. Not only containers, but also a command to open the settings page for
// containers is included in the menu model.
class ContainersMenuModel : public ui::SimpleMenuModel,
                            public ui::SimpleMenuModel::Delegate {
 public:
  ContainersMenuModel(ContainersMenuModelDelegate& delegate,
                      const PrefService& prefs);
  ~ContainersMenuModel() override;

  // ui::SimpleMenuModel::Delegate:
  void ExecuteCommand(int command_id, int event_flags) override;
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;

 private:
  friend class test::ContainersMenuModelTestApi;

  ContainersMenuModel(ContainersMenuModelDelegate& delegate,
                      std::vector<ContainerModel> items);

  void OpenContainerSettingsPage();
  void ContainerSelected(int command_id);

  int CommandIdToItemIndex(int command_id) const;
  int ItemIndexToCommandId(int item_index) const;

  base::raw_ref<ContainersMenuModelDelegate> delegate_;

  std::vector<ContainerModel> items_;
};

}  // namespace containers

#endif  // BRAVE_BROWSER_UI_CONTAINERS_CONTAINERS_MENU_MODEL_H_
