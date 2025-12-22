// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/containers/containers_menu_model_test_api.h"

#include <utility>

namespace containers::test {

// static
ContainersMenuModel ContainersMenuModelTestApi::CreateContainersMenuModel(
    ContainersMenuModelDelegate& delegate,
    std::vector<ContainerModel> containers) {
  return ContainersMenuModel(delegate, std::move(containers));
}

// static
const std::vector<ContainerModel>& ContainersMenuModelTestApi::GetItems(
    const ContainersMenuModel& model) {
  return model.items_;
}

// static
int ContainersMenuModelTestApi::GetCommandIdFromItemIndex(
    const ContainersMenuModel& model,
    int index) {
  return model.ItemIndexToCommandId(index);
}

// static
int ContainersMenuModelTestApi::GetItemIndexFromCommandId(
    const ContainersMenuModel& model,
    int command_id) {
  return model.CommandIdToItemIndex(command_id);
}

}  // namespace containers::test
