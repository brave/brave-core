// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/settings_page_handler.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/containers/core/browser/prefs.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace containers {

namespace {

class MockSettingsPage : public mojom::SettingsPage {
 public:
  MockSettingsPage() = default;
  ~MockSettingsPage() override = default;

  mojo::PendingRemote<mojom::SettingsPage> BindAndGetRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }

  MockSettingsPage* operator->() {
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
  mojo::Receiver<mojom::SettingsPage> receiver_{this};
  std::vector<mojom::ContainerPtr> last_containers_;
  int containers_changed_count_ = 0;
};

class MockDelegate : public SettingsPageHandler::Delegate {
 public:
  void RemoveContainerData(const std::string& id,
                           base::OnceClosure callback) override {
    last_removed_container_id_ = id;
    std::move(callback).Run();
  }

  const std::string& last_removed_container_id() const {
    return last_removed_container_id_;
  }

 private:
  std::string last_removed_container_id_;
};

}  // namespace

class ContainersSettingsPageHandlerTest : public testing::Test {
 public:
  void SetUp() override {
    RegisterProfilePrefs(prefs_.registry());

    auto delegate = std::make_unique<MockDelegate>();
    mock_delegate_ = delegate.get();

    handler_ = std::make_unique<SettingsPageHandler>(
        mock_page_.BindAndGetRemote(), &prefs_, std::move(delegate));
  }

  void TearDown() override { mock_delegate_ = nullptr; }

 protected:
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  MockSettingsPage mock_page_;
  raw_ptr<MockDelegate> mock_delegate_ = nullptr;
  std::unique_ptr<SettingsPageHandler> handler_;
};

TEST_F(ContainersSettingsPageHandlerTest, AddContainer) {
  handler_->AddOrUpdateContainer(mojom::Container::New("", "Test Container"));

  base::test::TestFuture<std::vector<mojom::ContainerPtr>> future;
  handler_->GetContainers(future.GetCallback());
  std::vector<mojom::ContainerPtr> containers = future.Take();
  ASSERT_EQ(1u, containers.size());
  EXPECT_FALSE(containers[0]->id.empty());  // Should have generated UUID
  EXPECT_EQ("Test Container", containers[0]->name);

  EXPECT_EQ(1, mock_page_->containers_changed_count());
}

TEST_F(ContainersSettingsPageHandlerTest, UpdateContainer) {
  // First add a container
  handler_->AddOrUpdateContainer(mojom::Container::New("", "Original Name"));

  // Get the container with generated ID
  base::test::TestFuture<std::vector<mojom::ContainerPtr>> future;
  handler_->GetContainers(future.GetCallback());
  std::vector<mojom::ContainerPtr> containers = future.Take();
  ASSERT_EQ(1u, containers.size());

  // Update the container
  auto updated = mojom::Container::New(containers[0]->id, "Updated Name");
  handler_->AddOrUpdateContainer(std::move(updated));

  // Verify update
  handler_->GetContainers(future.GetCallback());
  containers = future.Take();

  ASSERT_EQ(1u, containers.size());
  EXPECT_EQ("Updated Name", containers[0]->name);

  EXPECT_EQ(2, mock_page_->containers_changed_count());
}

TEST_F(ContainersSettingsPageHandlerTest, UpdateNonExistingContainerIsNoOp) {
  // Container ID is only generated on C++ side. It should be impossible to
  // add a container with ID passed from JS.
  handler_->AddOrUpdateContainer(
      mojom::Container::New("non-existing-id", "Updated Name"));

  base::test::TestFuture<std::vector<mojom::ContainerPtr>> future;
  handler_->GetContainers(future.GetCallback());
  std::vector<mojom::ContainerPtr> containers = future.Take();
  EXPECT_EQ(0u, containers.size());
}

TEST_F(ContainersSettingsPageHandlerTest, RemoveContainer) {
  // Add a container
  handler_->AddOrUpdateContainer(mojom::Container::New("", "Test Container"));

  // Get the container with generated ID
  std::string container_id;
  base::test::TestFuture<std::vector<mojom::ContainerPtr>> future;
  handler_->GetContainers(future.GetCallback());
  std::vector<mojom::ContainerPtr> containers = future.Take();
  ASSERT_EQ(1u, containers.size());
  container_id = containers[0]->id;

  // Remove the container
  base::test::TestFuture<void> future2;
  handler_->RemoveContainer(container_id, future2.GetCallback());
  future2.Get();
  EXPECT_EQ(container_id, mock_delegate_->last_removed_container_id());

  // Verify container was removed
  base::test::TestFuture<std::vector<mojom::ContainerPtr>> future3;
  handler_->GetContainers(future3.GetCallback());
  std::vector<mojom::ContainerPtr> containers2 = future3.Take();
  ASSERT_TRUE(containers2.empty());

  EXPECT_EQ(2, mock_page_->containers_changed_count());
}

TEST_F(ContainersSettingsPageHandlerTest, ExternalContainerChanges) {
  // Simulate external change to container list
  std::vector<mojom::ContainerPtr> containers;
  containers.push_back(mojom::Container::New("test-id", "Test Container"));
  SetContainerList(containers, prefs_);

  EXPECT_EQ(1, mock_page_->containers_changed_count());
  ASSERT_EQ(1u, mock_page_->last_containers().size());
  EXPECT_EQ("test-id", mock_page_->last_containers()[0]->id);
  EXPECT_EQ("Test Container", mock_page_->last_containers()[0]->name);
}

}  // namespace containers
