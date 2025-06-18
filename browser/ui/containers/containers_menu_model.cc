// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/containers/containers_menu_model.h"

#include "base/notimplemented.h"
#include "brave/browser/ui/containers/mock_containers_service.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/grit/generated_resources.h"

ContainersMenuModel::ContainersMenuModel(Type type, Delegate& delegate)
    : ui::SimpleMenuModel(this), type_(type), delegate_(delegate) {
  // 1. Add items for each container.
  int index = 0;
  for (const auto& container :
       MockContainersService::GetInstance().containers()) {
    AddCheckItem(container.id, container.name);
    SetIcon(index, container.icon);
    index++;
  }

  // 2. Add a separator.
  AddSeparator(ui::NORMAL_SEPARATOR);

  // 3. Add an item to open settings page.
  AddItemWithStringId(kCommandToOpenSettingsPage,
                      IDS_CXMENU_OPEN_CONTAINERS_SETTINGS);
}

ContainersMenuModel::~ContainersMenuModel() = default;

void ContainersMenuModel::ExecuteCommand(int command_id, int event_flags) {
  if (command_id == kCommandToOpenSettingsPage) {
    // Open the containers settings page
    OpenContainerSettingsPage();
    return;
  }

  // Set the selected container ID.
  ContainerSelected(command_id);
}

void ContainersMenuModel::OpenContainerSettingsPage() {
  NOTIMPLEMENTED();
}

void ContainersMenuModel::ContainerSelected(int container_id) {
  delegate_->OnContainerSelected(container_id);
}

bool ContainersMenuModel::IsCommandIdChecked(int command_id) const {
  if (type_ == Type::kLink) {
    // For links, we don't have a current container concept.
    return false;
  }

  return command_id ==
         MockContainersService::GetInstance().current_tab_container_id();
}

bool ContainersMenuModel::IsCommandIdEnabled(int command_id) const {
  return true;
}
