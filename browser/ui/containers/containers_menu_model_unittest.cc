// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/containers/containers_menu_model.h"

#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ui/containers/container_model.h"
#include "brave/browser/ui/containers/mock_containers_menu_model_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using containers::ContainerModel;
using containers::ContainersMenuModel;

class ContainersMenuModelUnitTest : public testing::Test {
 protected:
  std::vector<ContainerModel> GetContainers() const {
    std::vector<ContainerModel> containers;
    for (const auto& model : containers_) {
      containers.push_back(ContainerModel(model.container()->Clone(), {}));
    }
    return containers;
  }

  void SetUp() override {
    containers_.push_back(ContainerModel(
        containers::mojom::Container::New("ExampleContainer1", "Example 1"),
        {}));
    containers_.push_back(ContainerModel(
        containers::mojom::Container::New("ExampleContainer2", "Example 2"),
        {}));
    containers_.push_back(ContainerModel(
        containers::mojom::Container::New("ExampleContainer3", "Example 3"),
        {}));
  }

  void TearDown() override { containers_.clear(); }

 private:
  std::vector<ContainerModel> containers_;
};

MATCHER_P(EqId, id, "") {
  return arg->id == id;
}

TEST_F(ContainersMenuModelUnitTest, ModelContainsAllContainers) {
  MockContainerMenuModelDelegate delegate;

  ContainersMenuModel model(ContainersMenuModel::Type::kTab, delegate,
                            GetContainers());
  auto containers = GetContainers();

  // Verify the model contains all containers from the service
  for (size_t i = 0; i < containers.size(); ++i) {
    EXPECT_TRUE(model.GetIndexOfCommandId(i).has_value());
    EXPECT_EQ(containers.at(i).id(), model.items_.at(i).id());
    EXPECT_EQ(base::UTF8ToUTF16(containers.at(i).name()), model.GetLabelAt(i));
  }

  // Last item should be the "Manage Containers" command
  EXPECT_EQ(ContainersMenuModel::kCommandToOpenSettingsPage,
            model.GetCommandIdAt(model.GetItemCount() - 1));
}

TEST_F(ContainersMenuModelUnitTest, ExecuteCommandCallsDelegate) {
  MockContainerMenuModelDelegate delegate;
  ContainersMenuModel model(ContainersMenuModel::Type::kTab, delegate,
                            GetContainers());
  EXPECT_CALL(delegate, OnContainerSelected(EqId("ExampleContainer1")));
  model.ExecuteCommand(0, 0);
}

TEST_F(ContainersMenuModelUnitTest,
       TabTypeShouldReturnCheckedForCurrentContainer) {
  MockContainerMenuModelDelegate delegate;
  ContainersMenuModel model(ContainersMenuModel::Type::kTab, delegate,
                            GetContainers());

  auto containers = GetContainers();

  // Test with the first container ID
  EXPECT_CALL(delegate, GetCurrentContainerId())
      .WillRepeatedly(
          testing::Return(std::optional<std::string_view>(containers[0].id())));
  EXPECT_TRUE(model.IsCommandIdChecked(0));
  EXPECT_FALSE(model.IsCommandIdChecked(1));

  // Now test with the second container ID
  EXPECT_CALL(delegate, GetCurrentContainerId())
      .WillRepeatedly(
          testing::Return(std::optional<std::string_view>(containers[1].id())));
  EXPECT_FALSE(model.IsCommandIdChecked(0));
  EXPECT_TRUE(model.IsCommandIdChecked(1));

  // Test with no container selected
  EXPECT_CALL(delegate, GetCurrentContainerId())
      .WillRepeatedly(testing::Return(std::nullopt));
  EXPECT_FALSE(model.IsCommandIdChecked(0));
  EXPECT_FALSE(model.IsCommandIdChecked(1));
}

TEST_F(ContainersMenuModelUnitTest, LinkTypeNeverReturnsChecked) {
  MockContainerMenuModelDelegate delegate;
  ContainersMenuModel model(ContainersMenuModel::Type::kLink, delegate,
                            GetContainers());

  const auto& containers = GetContainers();
  EXPECT_CALL(delegate, GetCurrentContainerId())
      .WillRepeatedly(
          testing::Return(std::optional<std::string_view>(containers[0].id())));

  // For links, it'll open the link in a container, so we don't have a current
  // container.
  for (size_t i = 0; i < containers.size() - 1; ++i) {
    EXPECT_FALSE(model.IsCommandIdChecked(i));
  }
}
