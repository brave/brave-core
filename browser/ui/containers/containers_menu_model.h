// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_CONTAINERS_CONTAINERS_MENU_MODEL_H_
#define BRAVE_BROWSER_UI_CONTAINERS_CONTAINERS_MENU_MODEL_H_

#include <limits>

#include "base/memory/raw_ref.h"
#include "ui/menus/simple_menu_model.h"

class ContainersMenuModel : public ui::SimpleMenuModel,
                            public ui::SimpleMenuModel::Delegate {
 public:
  enum class Type {
    kTab,
    kLink,
  };

  class Delegate {
   public:
    Delegate() = default;
    Delegate(const Delegate&) = delete;
    Delegate& operator=(const Delegate&) = delete;
    virtual ~Delegate() = default;

    virtual void OnContainerSelected(int container_id) = 0;
  };

  static constexpr int kCommandToOpenSettingsPage =
      std::numeric_limits<int>::max();

  ContainersMenuModel(Type type, Delegate& delegate);
  ~ContainersMenuModel() override;

  // ui::SimpleMenuModel:
  void ExecuteCommand(int command_id, int event_flags) override;

  // ui::SimpleMenuModel::Delegate:
  bool IsCommandIdChecked(int command_id) const override;
  bool IsCommandIdEnabled(int command_id) const override;

 private:
  void OpenContainerSettingsPage();
  void ContainerSelected(int container_id);

  Type type_ = Type::kTab;
  base::raw_ref<Delegate> delegate_;
};

#endif  // BRAVE_BROWSER_UI_CONTAINERS_CONTAINERS_MENU_MODEL_H_
