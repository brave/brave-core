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

namespace containers {

class ContainersMenuModel : public ui::SimpleMenuModel,
                            public ui::SimpleMenuModel::Delegate {
 public:
  class Delegate {
   public:
    Delegate() = default;
    Delegate(const Delegate&) = delete;
    Delegate& operator=(const Delegate&) = delete;
    virtual ~Delegate() = default;

    virtual void OnContainerSelected(const mojom::ContainerPtr& container) = 0;
    virtual base::flat_set<std::string> GetCurrentContainerIds() = 0;
    virtual Browser* GetBrowserToOpenSettings() = 0;
  };

  static constexpr int kCommandToOpenSettingsPage =
      std::numeric_limits<int>::max();

  ContainersMenuModel(Delegate& delegate, std::vector<ContainerModel> items);
  ~ContainersMenuModel() override;

  // ui::SimpleMenuModel::Delegate:
  void ExecuteCommand(int command_id, int event_flags) override;
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;

 private:
  FRIEND_TEST_ALL_PREFIXES(ContainersMenuModelUnitTest,
                           ModelContainsAllContainers);

  void OpenContainerSettingsPage();
  void ContainerSelected(int command_id);

  base::raw_ref<Delegate> delegate_;

  std::vector<ContainerModel> items_;
};

}  // namespace containers

#endif  // BRAVE_BROWSER_UI_CONTAINERS_CONTAINERS_MENU_MODEL_H_
