// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/containers/containers_menu_model.h"

#include <utility>
#include <vector>

#include "base/notimplemented.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/grit/generated_resources.h"

ContainersMenuModel::ContainersMenuModel(Type type,
                                         Delegate& delegate,
                                         std::vector<ContainerModel> items)
    : ui::SimpleMenuModel(this),
      type_(type),
      delegate_(delegate),
      items_(std::move(items)) {
  // 1. Add items for each container.
  int index = 0;
  for (const auto& item : items_) {
    AddCheckItem(index, base::UTF8ToUTF16(item.name()));
    SetIcon(index, item.icon());
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

void ContainersMenuModel::ContainerSelected(int command_id) {
  delegate_->OnContainerSelected(items_[command_id].CloneContainer());
}

bool ContainersMenuModel::IsCommandIdChecked(int command_id) const {
  if (type_ == Type::kLink) {
    // For links, we don't have a current container concept.
    return false;
  }

  const auto& id = delegate_->GetCurrentContainerId();
  return id.has_value() && items_.at(command_id).id() == id.value();
}

bool ContainersMenuModel::IsCommandIdEnabled(int command_id) const {
  return true;
}
