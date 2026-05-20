/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/workspaces/workspace_service.h"

#include <memory>
#include <vector>

#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/task/bind_post_task.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "brave/browser/workspaces/features.h"
#include "brave/browser/workspaces/workspace_metadata.h"
#include "brave/browser/workspaces/workspace_service_factory.h"
#include "brave/browser/workspaces/workspace_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/sessions/core/command_storage_backend.h"
#include "components/sessions/core/session_id.h"
#include "components/sessions/core/session_service_commands.h"
#include "components/sessions/core/session_types.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
WorkspaceMetadata MakeMeta(const std::string& name,
                           int windows,
                           int tabs,
                           base::Time t) {
  WorkspaceMetadata meta{.name = name,
                         .modified_at = t,
                         .number_of_windows = windows,
                         .number_of_tabs = tabs};
  return meta;
}
}  // namespace

class WorkspaceServiceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(profile_manager_.SetUp());
    // Instantiate the factory so it registers with
    // BrowserContextDependencyManager before profile creation, ensuring
    // kWorkspacesMetadataPref is registered.
    WorkspaceServiceFactory::GetInstance();
    profile_ = profile_manager_.CreateTestingProfile("test");
    service_ = std::make_unique<WorkspaceService>(*profile_);
  }

  content::BrowserTaskEnvironment task_environment_;
  TestingProfileManager profile_manager_{TestingBrowserProcess::GetGlobal()};
  raw_ptr<Profile> profile_ = nullptr;
  std::unique_ptr<WorkspaceService> service_;
};

// ---- Metadata (pref) tests --------------------------------------------------

// No workspaces listed w/ empty profile
TEST_F(WorkspaceServiceTest, ListWorkspaces_InitiallyEmpty) {
  EXPECT_TRUE(service_->ListWorkspaces().empty());
}

// Verify saving preference adds workspace to list
TEST_F(WorkspaceServiceTest, SaveMetadata_AppearsInList) {
  base::Time t = base::Time::FromSecondsSinceUnixEpoch(1700000000.0);
  service_->SaveWorkspaceMetadata(MakeMeta("My Workspace", 2, 5, t));

  auto workspaces = service_->ListWorkspaces();
  ASSERT_EQ(workspaces.size(), 1u);
  EXPECT_EQ(workspaces[0].name, "My Workspace");
  EXPECT_EQ(workspaces[0].number_of_windows, 2);
  EXPECT_EQ(workspaces[0].number_of_tabs, 5);
  EXPECT_EQ(workspaces[0].modified_at,
            base::Time::FromSecondsSinceUnixEpoch(1700000000.0));
}

// Verify listing of workspaces orders by date modified desc
TEST_F(WorkspaceServiceTest, ListWorkspaces_SortedByModifiedAtDescending) {
  base::Time t1 = base::Time::FromSecondsSinceUnixEpoch(1700000000.0);
  base::Time t2 = base::Time::FromSecondsSinceUnixEpoch(1700000100.0);
  base::Time t3 = base::Time::FromSecondsSinceUnixEpoch(1699999900.0);
  service_->SaveWorkspaceMetadata(MakeMeta("Alpha", 1, 3, t1));
  service_->SaveWorkspaceMetadata(MakeMeta("Beta", 1, 5, t2));
  service_->SaveWorkspaceMetadata(MakeMeta("Gamma", 1, 2, t3));

  auto workspaces = service_->ListWorkspaces();
  ASSERT_EQ(workspaces.size(), 3u);
  EXPECT_EQ(workspaces[0].name, "Beta");
  EXPECT_EQ(workspaces[1].name, "Alpha");
  EXPECT_EQ(workspaces[2].name, "Gamma");
}

// Removing preference removes it from the list
TEST_F(WorkspaceServiceTest, RemoveMetadata_DisappearsFromList) {
  base::Time t = base::Time::FromSecondsSinceUnixEpoch(1700000000.0);
  service_->SaveWorkspaceMetadata(MakeMeta("Work", 1, 3, t));
  ASSERT_EQ(service_->ListWorkspaces().size(), 1u);

  service_->RemoveWorkspaceMetadata("Work");
  EXPECT_TRUE(service_->ListWorkspaces().empty());
}

// Saving a workspace over another one replaces it
TEST_F(WorkspaceServiceTest, SaveMetadata_SameNameOverwrites) {
  base::Time t1 = base::Time::FromSecondsSinceUnixEpoch(1700000000.0);
  base::Time t2 = base::Time::FromSecondsSinceUnixEpoch(1700000100.0);
  service_->SaveWorkspaceMetadata(MakeMeta("Work", 1, 3, t1));
  service_->SaveWorkspaceMetadata(MakeMeta("Work", 2, 7, t2));

  auto workspaces = service_->ListWorkspaces();
  ASSERT_EQ(workspaces.size(), 1u);
  EXPECT_EQ(workspaces[0].name, "Work");
  EXPECT_EQ(workspaces[0].number_of_windows, 2);
  EXPECT_EQ(workspaces[0].number_of_tabs, 7);
}

// ---- Name / path computation tests -----------------------------------------

// Key is a stable hex hash — different display names get different paths.
TEST_F(WorkspaceServiceTest,
       GetWorkspacePathForName_DifferentNamesDifferentPaths) {
  base::FilePath path1 = service_->GetWorkspacePathForName("Work Space");
  base::FilePath path2 = service_->GetWorkspacePathForName("Work-Space");
  EXPECT_NE(path1, path2);
}

// Same name always resolves to the same path (deterministic key).
TEST_F(WorkspaceServiceTest, GetWorkspacePathForName_SameNameSamePath) {
  EXPECT_EQ(service_->GetWorkspacePathForName("My Workspace"),
            service_->GetWorkspacePathForName("My Workspace"));
}

// ---- Disk I/O tests (async, use RunUntil not RunUntilIdle) ------------------

// Verify that WriteWorkspaceToDisk saves the profile without calling onError.
TEST_F(WorkspaceServiceTest, WriteAndReadRoundTrip) {
  using CommandList = std::vector<std::unique_ptr<sessions::SessionCommand>>;

  SessionID window_id = SessionID::NewUnique();
  CommandList commands;
  commands.push_back(sessions::CreateSetWindowTypeCommand(
      window_id, sessions::SessionWindow::TYPE_NORMAL));
  commands.push_back(
      sessions::CreateSetSelectedTabInWindowCommand(window_id, 0));
  const size_t num_commands = commands.size();

  base::FilePath workspace_path =
      service_->GetWorkspacePathForName("roundtrip");
  auto task_runner = base::ThreadPool::CreateSequencedTaskRunner(
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN});
  auto backend = base::MakeRefCounted<sessions::CommandStorageBackend>(
      task_runner, workspace_path, kWorkspaceSessionType,
      /*encryptor=*/std::nullopt);

  bool error_called = false;
  bool write_done = false;
  auto on_error = base::BindPostTask(
      base::SequencedTaskRunner::GetCurrentDefault(),
      base::BindOnce([](bool* flag) { *flag = true; }, &error_called));

  task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&WriteWorkspaceToDisk, std::move(commands), workspace_path,
                     std::move(backend), std::move(on_error)));
  // A sequenced task after the write runs only after it completes.
  task_runner->PostTask(
      FROM_HERE,
      base::BindPostTask(
          base::SequencedTaskRunner::GetCurrentDefault(),
          base::BindOnce([](bool* flag) { *flag = true; }, &write_done)));

  ASSERT_TRUE(base::test::RunUntil([&] { return write_done; }));
  ASSERT_FALSE(error_called);

  // Read back using a fresh backend (mirrors RestoreWorkspace's pattern).
  auto read_backend = base::MakeRefCounted<sessions::CommandStorageBackend>(
      task_runner, workspace_path, kWorkspaceSessionType,
      /*encryptor=*/std::nullopt);
  base::test::TestFuture<CommandList> read_future;
  task_runner->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&ReadWorkspaceFromDisk, workspace_path, read_backend),
      read_future.GetCallback());

  ASSERT_TRUE(read_future.Wait());
  const auto& read_cmds = read_future.Get();
  ASSERT_EQ(read_cmds.size(), num_commands);

  // Verify content — not just count — survived the round-trip.
  // Re-creating the reference commands from the same inputs and comparing
  // serialized bytes confirms the window ID and TYPE_NORMAL encoding are
  // preserved end-to-end (both are embedded in the serialized payload).
  EXPECT_EQ(read_cmds[0]->Serialize(),
            sessions::CreateSetWindowTypeCommand(
                window_id, sessions::SessionWindow::TYPE_NORMAL)
                ->Serialize());
  EXPECT_EQ(
      read_cmds[1]->Serialize(),
      sessions::CreateSetSelectedTabInWindowCommand(window_id, 0)->Serialize());
}

TEST_F(WorkspaceServiceTest,
       WriteToDisk_DirectoryCreationFailure_CallsOnError) {
  // Place a regular FILE where the workspace directory would be so that
  // base::CreateDirectory fails on the background thread and on_error is
  // posted back to the UI thread.
  ASSERT_TRUE(base::CreateDirectory(service_->GetWorkspacesPath()));
  base::FilePath workspace_path = service_->GetWorkspacePathForName("blocked");
  ASSERT_TRUE(base::WriteFile(workspace_path, "not-a-directory"));

  auto task_runner = base::ThreadPool::CreateSequencedTaskRunner(
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN});
  auto backend = base::MakeRefCounted<sessions::CommandStorageBackend>(
      task_runner, workspace_path, kWorkspaceSessionType,
      /*encryptor=*/std::nullopt);

  bool error_called = false;
  auto on_error = base::BindPostTask(
      base::SequencedTaskRunner::GetCurrentDefault(),
      base::BindOnce([](bool* flag) { *flag = true; }, &error_called));

  task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&WriteWorkspaceToDisk,
                     std::vector<std::unique_ptr<sessions::SessionCommand>>(),
                     workspace_path, std::move(backend), std::move(on_error)));

  EXPECT_TRUE(base::test::RunUntil([&] { return error_called; }));
}

// ---- Factory tests ----------------------------------------------------------

class WorkspaceServiceFactoryTest : public ::testing::Test {
 protected:
  void SetUp() override { ASSERT_TRUE(profile_manager_.SetUp()); }

  content::BrowserTaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_;
  TestingProfileManager profile_manager_{TestingBrowserProcess::GetGlobal()};
};

TEST_F(WorkspaceServiceFactoryTest, FeatureDisabled_GetForProfileReturnsNull) {
  feature_list_.InitAndDisableFeature(features::kWorkspaces);
  auto* profile = profile_manager_.CreateTestingProfile("test");
  EXPECT_EQ(WorkspaceServiceFactory::GetForProfile(profile), nullptr);
}

TEST_F(WorkspaceServiceFactoryTest,
       FeatureEnabled_GetForProfileReturnsNonNull) {
  feature_list_.InitAndEnableFeature(features::kWorkspaces);
  auto* profile = profile_manager_.CreateTestingProfile("test");
  // ServiceIsNULLWhileTesting() suppresses service creation for test profiles.
  // Provide a testing factory so the feature-enabled path is exercised.
  WorkspaceServiceFactory::GetInstance()->SetTestingFactory(
      profile,
      base::BindLambdaForTesting(
          [](content::BrowserContext* ctx) -> std::unique_ptr<KeyedService> {
            auto* p = Profile::FromBrowserContext(ctx);
            return std::make_unique<WorkspaceService>(*p);
          }));
  EXPECT_NE(WorkspaceServiceFactory::GetForProfile(profile), nullptr);
}
