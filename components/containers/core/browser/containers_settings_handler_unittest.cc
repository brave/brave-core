// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/containers_settings_handler.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/containers/core/browser/prefs.h"
#include "brave/components/containers/core/common/features.h"
#include "brave/components/containers/core/mojom/containers.mojom-data-view.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace containers {

namespace {

constexpr struct {
  const char* name;
  const bool is_valid;
} kContainerTestNames[] = {
    {"Test", true},
    {"Test Container", true},
    {"  Test Container  ", true},
    {"  Test  Container  ", true},
    {"", false},
    {"   ", false},
    {"\n\t\r", false},
    {" \n\t\r ", false},
    {"  Test \n \r \t Container  ", false},
};

class MockContainersSettingsObserver : public mojom::ContainersSettingsUI {
 public:
  MockContainersSettingsObserver() = default;
  ~MockContainersSettingsObserver() override = default;

  mojo::PendingRemote<mojom::ContainersSettingsUI> BindAndGetRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }

  MockContainersSettingsObserver* operator->() {
    if (receiver_.is_bound()) {
      receiver_.FlushForTesting();
    }
    return this;
  }

  void OnContainersChanged(
      std::vector<mojom::ContainerPtr> containers) override {
    last_containers_ = std::move(containers);
    containers_changed_count_++;
  }

  const std::vector<mojom::ContainerPtr>& last_containers() const {
    return last_containers_;
  }

  int containers_changed_count() const { return containers_changed_count_; }

 private:
  mojo::Receiver<mojom::ContainersSettingsUI> receiver_{this};
  std::vector<mojom::ContainerPtr> last_containers_;
  int containers_changed_count_ = 0;
};

}  // namespace

class ContainersSettingsHandlerTest : public testing::Test {
 public:
  void SetUp() override {
    feature_list_.InitAndEnableFeature(features::kContainers);
    RegisterProfilePrefs(prefs_.registry());

    handler_ = std::make_unique<ContainersSettingsHandler>(&prefs_);
    handler_->BindUI(mock_observer_.BindAndGetRemote());
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  MockContainersSettingsObserver mock_observer_;
  std::unique_ptr<ContainersSettingsHandler> handler_;
};

TEST_F(ContainersSettingsHandlerTest, ValidateContainerProperties) {
  // Valid properties
  EXPECT_EQ(ContainersSettingsHandler::ValidateEditableContainerProperties(
                mojom::Container::New("id", "Valid Name",
                                      mojom::Icon::kPersonal, SK_ColorWHITE)),
            std::nullopt);

  // Valid/Invalid names
  for (const auto& test_name : kContainerTestNames) {
    if (test_name.is_valid) {
      EXPECT_THAT(
          ContainersSettingsHandler::ValidateEditableContainerProperties(
              mojom::Container::New("id", test_name.name,
                                    mojom::Icon::kPersonal, SK_ColorWHITE)),
          std::nullopt);
    } else {
      EXPECT_THAT(
          ContainersSettingsHandler::ValidateEditableContainerProperties(
              mojom::Container::New("id", test_name.name,
                                    mojom::Icon::kPersonal, SK_ColorWHITE)),
          testing::Optional(mojom::ContainerOperationError::kInvalidName));
    }
  }

  // Invalid icons
  for (const auto& icon : {std::to_underlying(mojom::Icon::kMinValue) - 1,
                           std::to_underlying(mojom::Icon::kMaxValue) + 1}) {
    EXPECT_THAT(
        ContainersSettingsHandler::ValidateEditableContainerProperties(
            mojom::Container::New("id", "Valid Name", mojom::Icon(icon),
                                  SK_ColorWHITE)),
        testing::Optional(mojom::ContainerOperationError::kInvalidIcon));
  }

  // Valid icons
  for (const auto& icon : {mojom::Icon::kPersonal, mojom::Icon::kBanking,
                           mojom::Icon::kShopping}) {
    EXPECT_THAT(
        ContainersSettingsHandler::ValidateEditableContainerProperties(
            mojom::Container::New("id", "Valid Name", icon, SK_ColorWHITE)),
        std::nullopt);
  }

  // Invalid background colors
  for (const auto& color :
       {SK_ColorBLACK - 1, SK_ColorWHITE + 1, SkColorSetA(SK_ColorWHITE, 0)}) {
    EXPECT_THAT(ContainersSettingsHandler::ValidateEditableContainerProperties(
                    mojom::Container::New("id", "Valid Name",
                                          mojom::Icon::kPersonal, color)),
                testing::Optional(
                    mojom::ContainerOperationError::kInvalidBackgroundColor));
  }

  // Valid background colors
  for (const auto& color : {SK_ColorWHITE, SK_ColorBLACK, SK_ColorRED,
                            SK_ColorGREEN, SK_ColorBLUE}) {
    EXPECT_THAT(ContainersSettingsHandler::ValidateEditableContainerProperties(
                    mojom::Container::New("id", "Valid Name",
                                          mojom::Icon::kPersonal, color)),
                std::nullopt);
  }
}

TEST_F(ContainersSettingsHandlerTest, AddContainer) {
  base::test::TestFuture<std::optional<mojom::ContainerOperationError>>
      error_future;
  handler_->AddContainer(
      mojom::Container::New("", "Test Container", mojom::Icon::kPersonal,
                            SK_ColorWHITE),
      error_future.GetCallback());
  EXPECT_EQ(std::nullopt, error_future.Take());

  base::test::TestFuture<std::vector<mojom::ContainerPtr>> future;
  handler_->GetContainers(future.GetCallback());
  std::vector<mojom::ContainerPtr> containers = future.Take();
  ASSERT_EQ(1u, containers.size());
  EXPECT_FALSE(containers[0]->id.empty());  // Should have generated UUID
  EXPECT_EQ("Test Container", containers[0]->name);
  EXPECT_EQ(mojom::Icon::kPersonal, containers[0]->icon);
  EXPECT_EQ(SK_ColorWHITE, containers[0]->background_color);

  EXPECT_EQ(1, mock_observer_->containers_changed_count());

  // Invalid ID.
  handler_->AddContainer(
      mojom::Container::New("test-id", "", mojom::Icon::kPersonal,
                            SK_ColorWHITE),
      error_future.GetCallback());
  EXPECT_EQ(mojom::ContainerOperationError::kIdShouldBeEmpty,
            error_future.Take());

  // Invalid name.
  for (const auto& test_name : kContainerTestNames) {
    handler_->AddContainer(
        mojom::Container::New("", test_name.name, mojom::Icon::kPersonal,
                              SK_ColorWHITE),
        error_future.GetCallback());
    EXPECT_EQ(test_name.is_valid
                  ? std::nullopt
                  : std::optional<mojom::ContainerOperationError>(
                        mojom::ContainerOperationError::kInvalidName),
              error_future.Take());
  }

  // Invalid icon.
  for (const auto& icon : {std::to_underlying(mojom::Icon::kMinValue) - 1,
                           std::to_underlying(mojom::Icon::kMaxValue) + 1}) {
    handler_->AddContainer(
        mojom::Container::New("", "Invalid icon", mojom::Icon(icon),
                              SK_ColorWHITE),
        error_future.GetCallback());
    EXPECT_EQ(mojom::ContainerOperationError::kInvalidIcon,
              error_future.Take());
  }

  // Invalid background color.
  for (const auto& color :
       {SK_ColorBLACK - 1, SK_ColorWHITE + 1, SkColorSetA(SK_ColorWHITE, 0)}) {
    handler_->AddContainer(mojom::Container::New("", "Invalid icon",
                                                 mojom::Icon::kPersonal, color),
                           error_future.GetCallback());
    EXPECT_EQ(mojom::ContainerOperationError::kInvalidBackgroundColor,
              error_future.Take());
  }
}

TEST_F(ContainersSettingsHandlerTest, UpdateContainer) {
  // First add a container
  base::test::TestFuture<std::optional<mojom::ContainerOperationError>>
      error_future;
  handler_->AddContainer(
      mojom::Container::New("", "Original Name", mojom::Icon::kPersonal,
                            SK_ColorWHITE),
      error_future.GetCallback());
  EXPECT_EQ(std::nullopt, error_future.Take());

  // Get the container with generated ID
  base::test::TestFuture<std::vector<mojom::ContainerPtr>> future;
  handler_->GetContainers(future.GetCallback());
  std::vector<mojom::ContainerPtr> containers = future.Take();
  ASSERT_EQ(1u, containers.size());

  // Update the container
  auto updated = mojom::Container::New(containers[0]->id, "Updated Name",
                                       mojom::Icon::kBanking, SK_ColorMAGENTA);
  handler_->UpdateContainer(std::move(updated), error_future.GetCallback());
  EXPECT_EQ(std::nullopt, error_future.Take());

  // Verify update
  handler_->GetContainers(future.GetCallback());
  containers = future.Take();

  ASSERT_EQ(1u, containers.size());
  EXPECT_EQ("Updated Name", containers[0]->name);
  EXPECT_EQ(mojom::Icon::kBanking, containers[0]->icon);
  EXPECT_EQ(SK_ColorMAGENTA, containers[0]->background_color);

  EXPECT_EQ(2, mock_observer_->containers_changed_count());

  // Not found.
  handler_->UpdateContainer(
      mojom::Container::New("non-existing-id", "name", mojom::Icon::kPersonal,
                            SK_ColorWHITE),
      error_future.GetCallback());
  EXPECT_EQ(mojom::ContainerOperationError::kNotFound, error_future.Take());

  // Empty ID.
  handler_->UpdateContainer(
      mojom::Container::New("", "name", mojom::Icon::kPersonal, SK_ColorWHITE),
      error_future.GetCallback());
  EXPECT_EQ(mojom::ContainerOperationError::kIdShouldBeSet,
            error_future.Take());

  // Invalid name.
  for (const auto& test_name : kContainerTestNames) {
    handler_->UpdateContainer(
        mojom::Container::New(containers[0]->id, test_name.name,
                              mojom::Icon::kPersonal, SK_ColorWHITE),
        error_future.GetCallback());
    EXPECT_EQ(test_name.is_valid
                  ? std::nullopt
                  : std::optional<mojom::ContainerOperationError>(
                        mojom::ContainerOperationError::kInvalidName),
              error_future.Take());
  }

  // Invalid icon.
  for (const auto& icon : {std::to_underlying(mojom::Icon::kMinValue) - 1,
                           std::to_underlying(mojom::Icon::kMaxValue) + 1}) {
    handler_->UpdateContainer(
        mojom::Container::New(containers[0]->id, "Invalid icon",
                              mojom::Icon(icon), SK_ColorWHITE),
        error_future.GetCallback());
    EXPECT_EQ(mojom::ContainerOperationError::kInvalidIcon,
              error_future.Take());
  }

  // Invalid background color.
  for (const auto& color :
       {SK_ColorBLACK - 1, SK_ColorWHITE + 1, SkColorSetA(SK_ColorWHITE, 0)}) {
    handler_->UpdateContainer(
        mojom::Container::New(containers[0]->id, "Invalid icon",
                              mojom::Icon::kPersonal, color),
        error_future.GetCallback());
    EXPECT_EQ(mojom::ContainerOperationError::kInvalidBackgroundColor,
              error_future.Take());
  }
}

TEST_F(ContainersSettingsHandlerTest, RemoveContainer) {
  // Add a container
  base::test::TestFuture<std::optional<mojom::ContainerOperationError>>
      error_future;
  handler_->AddContainer(
      mojom::Container::New("", "Test Container", mojom::Icon::kPersonal,
                            SK_ColorWHITE),
      error_future.GetCallback());
  EXPECT_EQ(std::nullopt, error_future.Take());

  // Get the container with generated ID
  std::string container_id;
  base::test::TestFuture<std::vector<mojom::ContainerPtr>> future;
  handler_->GetContainers(future.GetCallback());
  std::vector<mojom::ContainerPtr> containers = future.Take();
  ASSERT_EQ(1u, containers.size());
  container_id = containers[0]->id;

  // Remove the container
  handler_->RemoveContainer(container_id, error_future.GetCallback());
  ASSERT_EQ(std::nullopt, error_future.Take());

  // Verify container was removed
  handler_->GetContainers(future.GetCallback());
  containers = future.Take();
  ASSERT_TRUE(containers.empty());

  EXPECT_EQ(2, mock_observer_->containers_changed_count());
}

TEST_F(ContainersSettingsHandlerTest, RemoveInvalidContainer) {
  base::test::TestFuture<std::optional<mojom::ContainerOperationError>>
      error_future;
  handler_->RemoveContainer("", error_future.GetCallback());
  EXPECT_EQ(mojom::ContainerOperationError::kIdShouldBeSet,
            error_future.Take());

  handler_->RemoveContainer("non-existing-id", error_future.GetCallback());
  EXPECT_EQ(mojom::ContainerOperationError::kNotFound, error_future.Take());
}

TEST_F(ContainersSettingsHandlerTest, ExternalContainerChanges) {
  // Simulate external change to container list
  std::vector<mojom::ContainerPtr> containers;
  containers.push_back(mojom::Container::New(
      "test-id", "Test Container", mojom::Icon::kPersonal, SK_ColorWHITE));
  SetContainersToPrefs(containers, prefs_);

  EXPECT_EQ(1, mock_observer_->containers_changed_count());
  ASSERT_EQ(1u, mock_observer_->last_containers().size());
  EXPECT_EQ("test-id", mock_observer_->last_containers()[0]->id);
  EXPECT_EQ("Test Container", mock_observer_->last_containers()[0]->name);
}

}  // namespace containers
