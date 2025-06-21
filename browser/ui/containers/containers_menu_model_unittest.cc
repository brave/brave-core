// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/containers/containers_menu_model.h"

#include "brave/browser/ui/containers/mock_containers_menu_model_delegate.h"
#include "brave/browser/ui/containers/mock_containers_service.h"
#include "testing/gtest/include/gtest/gtest.h"

using ContainersMenuModelUnittest = testing::Test;

TEST_F(ContainersMenuModelUnittest, ModelContainsAllContainers) {
  MockContainersService& service = MockContainersService::GetInstance();
  MockContainerMenuModelDelegate delegate;
  ContainersMenuModel model(ContainersMenuModel::Type::kTab, delegate);

  // Verify the model contains all containers from the service
  const auto& containers = service.containers();
  for (const auto& container : containers) {
    EXPECT_TRUE(model.GetIndexOfCommandId(container.id).has_value());
  }

  // Last item should be the "Manage Containers" command
  EXPECT_EQ(ContainersMenuModel::kCommandToOpenSettingsPage,
            model.GetCommandIdAt(model.GetItemCount() - 1));
}

TEST_F(ContainersMenuModelUnittest, ExecuteCommandCallsDelegate) {
  MockContainerMenuModelDelegate delegate;
  ContainersMenuModel model(ContainersMenuModel::Type::kTab, delegate);
  EXPECT_CALL(delegate, OnContainerSelected(1));
  model.ExecuteCommand(1, 0);
}

TEST_F(ContainersMenuModelUnittest,
       TabTypeShouldReturnCheckedForCurrentContainer) {
  MockContainerMenuModelDelegate delegate;
  ContainersMenuModel model(ContainersMenuModel::Type::kTab, delegate);

  MockContainersService& service = MockContainersService::GetInstance();
  ASSERT_EQ(-1, service.current_tab_container_id());
  for (const auto& container : service.containers()) {
    ASSERT_FALSE(model.IsCommandIdChecked(container.id));
  }

  // Set current tab container ID
  service.set_selected_container_id(0);
  EXPECT_TRUE(model.IsCommandIdChecked(0));
  EXPECT_FALSE(model.IsCommandIdChecked(1));
}

TEST_F(ContainersMenuModelUnittest, LinkTypeNeverReturnsChecked) {
  MockContainerMenuModelDelegate delegate;
  ContainersMenuModel model(ContainersMenuModel::Type::kLink, delegate);

  MockContainersService& service = MockContainersService::GetInstance();
  service.set_selected_container_id(0);

  // For links, it'll open the link in a container, so we don't have a current
  // container.
  for (const auto& container : service.containers()) {
    EXPECT_FALSE(model.IsCommandIdChecked(container.id));
  }
}
