// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/containers/containers_menu_model.h"

#include <utility>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "brave/app/brave_command_ids.h"
#include "brave/components/containers/core/browser/prefs.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/models/image_model.h"

#if BUILDFLAG(IS_MAC)
#include "ui/gfx/image/image_skia.h"
#endif

namespace containers {

namespace {

std::vector<ContainerModel> GetContainerModelsFromPrefs(
    const PrefService& prefs,
    float scale_factor) {
  std::vector<ContainerModel> containers;
  for (auto& container : GetContainersFromPrefs(prefs)) {
    containers.emplace_back(std::move(container), scale_factor);
  }
  return containers;
}

}  // namespace

ContainersMenuModel::ContainersMenuModel(Delegate& delegate,
                                         const PrefService& prefs)
    : ContainersMenuModel(
          delegate,
          GetContainerModelsFromPrefs(prefs, delegate.GetScaleFactor())) {}

ContainersMenuModel::ContainersMenuModel(Delegate& delegate,
                                         std::vector<ContainerModel> items)
    : ui::SimpleMenuModel(this), delegate_(delegate), items_(std::move(items)) {
  // Trim the items to fit within the command ID range.
  const auto max_items = static_cast<size_t>(IDC_OPEN_IN_CONTAINER_END -
                                             IDC_OPEN_IN_CONTAINER_START + 1);
  if (items_.size() > max_items) {
    items_.erase(items_.begin() + max_items, items_.end());
    LOG(WARNING) << "Too many containers for the current menu model. "
                 << "Trimming to fit within command ID range.";
  }

  // 1. Add items for each container.
  int index = 0;
  for (const auto& item : items_) {
    AddCheckItem(ItemIndexToCommandId(index), base::UTF8ToUTF16(item.name()));
    SetIcon(index,
#if BUILDFLAG(IS_MAC)
            // On macOS, vector icon version of menu items are not supported.
            // For the reference, we tried fix this with adhoc patch in
            // https://github.com/brave/brave-core/pull/21835/files but was
            // imperfect in some cases.
            // So we just rasterize the icon right away and convert it to
            // ImageSkia version model.
            ui::ImageModel::FromImageSkia(
                item.icon().Rasterize(/*color_provider=*/nullptr)));
#else
            item.icon());
#endif
    index++;
  }

  // 2. Add a separator.
  AddSeparator(ui::NORMAL_SEPARATOR);

  // 3. Add an item to open settings page.
  AddItemWithStringId(IDC_OPEN_CONTAINERS_SETTING,
                      IDS_CXMENU_OPEN_CONTAINERS_SETTINGS);
}

ContainersMenuModel::~ContainersMenuModel() = default;

void ContainersMenuModel::ExecuteCommand(int command_id, int event_flags) {
  if (command_id == IDC_OPEN_CONTAINERS_SETTING) {
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
  delegate_->OnContainerSelected(
      items_[CommandIdToItemIndex(command_id)].container());
}

bool ContainersMenuModel::IsCommandIdChecked(int command_id) const {
  const auto& ids = delegate_->GetCurrentContainerIds();
  return ids.contains(items_[CommandIdToItemIndex(command_id)].container()->id);
}

bool ContainersMenuModel::IsCommandIdEnabled(int command_id) const {
  return true;
}

int ContainersMenuModel::CommandIdToItemIndex(int command_id) const {
  const auto item_index = command_id - IDC_OPEN_IN_CONTAINER_START;
  CHECK(item_index >= 0 && item_index < static_cast<int>(items_.size()))
      << "Command ID " << command_id
      << " is out of range for the current menu model.";
  return item_index;
}

int ContainersMenuModel::ItemIndexToCommandId(int item_index) const {
  CHECK(item_index >= 0 && item_index < static_cast<int>(items_.size()))
      << "Item index " << item_index
      << " is out of range for the current menu model.";
  return item_index + IDC_OPEN_IN_CONTAINER_START;
}

}  // namespace containers
