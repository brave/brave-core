// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/containers/containers_menu_model.h"

#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/containers/container_model.h"
#include "brave/browser/ui/containers/containers_menu_model_test_api.h"
#include "brave/browser/ui/containers/mock_containers_menu_model_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace containers {

class ContainersMenuModelUnitTest : public testing::Test {
 protected:
  std::vector<ContainerModel> GetContainers() const {
    std::vector<ContainerModel> containers;
    for (const auto& model : containers_) {
      containers.emplace_back(model.container()->Clone(),
                              /*scale_factor=*/1.0f);
    }
    return containers;
  }

  void SetUp() override {
    containers_.emplace_back(
        mojom::Container::New("ExampleContainer1", "Example 1",
                              mojom::Icon::kPersonal, SK_ColorWHITE),
        /*scale_factor=*/1.0f);
    containers_.emplace_back(
        mojom::Container::New("ExampleContainer2", "Example 2",
                              mojom::Icon::kWork, SK_ColorBLACK),
        /*scale_factor=*/1.0f);
    containers_.emplace_back(
        mojom::Container::New("ExampleContainer3", "Example 3",
                              mojom::Icon::kPersonal, SK_ColorWHITE),
        /*scale_factor=*/1.0f);
  }

  void TearDown() override { containers_.clear(); }

 private:
  std::vector<ContainerModel> containers_;
};

MATCHER_P(EqId, id, "") {
  return arg->id == id;
}

TEST_F(ContainersMenuModelUnitTest, ModelContainsAllContainers) {
  MockContainersMenuModelDelegate delegate;

  ContainersMenuModel model =
      test::ContainersMenuModelTestApi::CreateContainersMenuModel(
          delegate, GetContainers());
  auto containers = GetContainers();

  // Verify the model contains all containers from the service
  for (size_t i = 0; i < containers.size(); ++i) {
    const auto command_id =
        test::ContainersMenuModelTestApi::GetCommandIdFromItemIndex(model, i);
    EXPECT_TRUE(model.GetIndexOfCommandId(command_id).has_value());
    EXPECT_EQ(containers.at(i).id(),
              test::ContainersMenuModelTestApi::GetItems(model).at(i).id());
    EXPECT_EQ(base::UTF8ToUTF16(containers.at(i).name()), model.GetLabelAt(i));
  }

  // Last item should be the "Manage Containers" command
  EXPECT_EQ(IDC_OPEN_CONTAINERS_SETTING,
            model.GetCommandIdAt(model.GetItemCount() - 1));
}

TEST_F(ContainersMenuModelUnitTest, ExecuteCommandCallsDelegate) {
  MockContainersMenuModelDelegate delegate;
  ContainersMenuModel model =
      test::ContainersMenuModelTestApi::CreateContainersMenuModel(
          delegate, GetContainers());
  EXPECT_CALL(delegate, OnContainerSelected(EqId("ExampleContainer1")));
  model.ExecuteCommand(
      test::ContainersMenuModelTestApi::GetCommandIdFromItemIndex(model, 0), 0);
}

TEST_F(ContainersMenuModelUnitTest, GetCurrentContainerIdsAreChecked) {
  auto containers = GetContainers();

  // Test with the first container ID
  {
    MockContainersMenuModelDelegate delegate;
    EXPECT_CALL(delegate, GetCurrentContainerIds())
        .WillOnce(
            testing::Return((base::flat_set<std::string>{containers[0].id()})));
    ContainersMenuModel model =
        test::ContainersMenuModelTestApi::CreateContainersMenuModel(
            delegate, GetContainers());
    EXPECT_TRUE(model.IsCommandIdChecked(
        test::ContainersMenuModelTestApi::GetCommandIdFromItemIndex(model, 0)));
    EXPECT_FALSE(model.IsCommandIdChecked(
        test::ContainersMenuModelTestApi::GetCommandIdFromItemIndex(model, 1)));
  }

  // Now test with the second container ID
  {
    MockContainersMenuModelDelegate delegate;
    EXPECT_CALL(delegate, GetCurrentContainerIds())
        .WillOnce(
            testing::Return(base::flat_set<std::string>{containers[1].id()}));
    ContainersMenuModel model =
        test::ContainersMenuModelTestApi::CreateContainersMenuModel(
            delegate, GetContainers());
    EXPECT_FALSE(model.IsCommandIdChecked(
        test::ContainersMenuModelTestApi::GetCommandIdFromItemIndex(model, 0)));
    EXPECT_TRUE(model.IsCommandIdChecked(
        test::ContainersMenuModelTestApi::GetCommandIdFromItemIndex(model, 1)));
  }

  // Test with no container selected
  {
    MockContainersMenuModelDelegate delegate;
    EXPECT_CALL(delegate, GetCurrentContainerIds())
        .WillOnce(testing::Return<base::flat_set<std::string>>({}));
    ContainersMenuModel model =
        test::ContainersMenuModelTestApi::CreateContainersMenuModel(
            delegate, GetContainers());
    EXPECT_FALSE(model.IsCommandIdChecked(
        test::ContainersMenuModelTestApi::GetCommandIdFromItemIndex(model, 0)));
    EXPECT_FALSE(model.IsCommandIdChecked(
        test::ContainersMenuModelTestApi::GetCommandIdFromItemIndex(model, 1)));
  }

  // Test with multiple containers selected
  {
    MockContainersMenuModelDelegate delegate;
    EXPECT_CALL(delegate, GetCurrentContainerIds())
        .WillOnce(testing::Return(base::flat_set<std::string>{
            containers[0].id(), containers[1].id()}));
    ContainersMenuModel model =
        test::ContainersMenuModelTestApi::CreateContainersMenuModel(
            delegate, GetContainers());
    EXPECT_TRUE(model.IsCommandIdChecked(
        test::ContainersMenuModelTestApi::GetCommandIdFromItemIndex(model, 0)));
    EXPECT_TRUE(model.IsCommandIdChecked(
        test::ContainersMenuModelTestApi::GetCommandIdFromItemIndex(model, 1)));
    EXPECT_FALSE(model.IsCommandIdChecked(
        test::ContainersMenuModelTestApi::GetCommandIdFromItemIndex(model, 2)));
  }
}

}  // namespace containers
