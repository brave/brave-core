// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/containers/containers_menu_model.h"

#include <utility>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "brave/components/containers/core/browser/prefs.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/grit/generated_resources.h"

namespace containers {

namespace {

std::vector<ContainerModel> GetContainerModelsFromPrefs(
    const PrefService& prefs) {
  std::vector<ContainerModel> containers;
  for (auto& container : GetContainersFromPrefs(prefs)) {
    containers.emplace_back(std::move(container));
  }
  return containers;
}

}  // namespace

ContainersMenuModel::ContainersMenuModel(Delegate& delegate,
                                         const PrefService& prefs)
    : ContainersMenuModel(delegate, GetContainerModelsFromPrefs(prefs)) {}

ContainersMenuModel::ContainersMenuModel(Delegate& delegate,
                                         std::vector<ContainerModel> items)
    : ui::SimpleMenuModel(this), delegate_(delegate), items_(std::move(items)) {
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
  chrome::ShowSettingsSubPage(delegate_->GetBrowserToOpenSettings(),
                              "braveContent");
}

void ContainersMenuModel::ContainerSelected(int command_id) {
  delegate_->OnContainerSelected(items_[command_id].container());
}

bool ContainersMenuModel::IsCommandIdChecked(int command_id) const {
  const auto& ids = delegate_->GetCurrentContainerIds();
  return ids.contains(items_[command_id].container()->id);
}

bool ContainersMenuModel::IsCommandIdEnabled(int command_id) const {
  return true;
}

}  // namespace containers
