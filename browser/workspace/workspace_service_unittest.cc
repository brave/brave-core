/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/workspace/workspace_service.h"

#include <memory>
#include <vector>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/task/bind_post_task.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "brave/browser/workspace/features.h"
#include "brave/browser/workspace/workspace.h"
#include "brave/browser/workspace/workspace_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/sessions/core/command_storage_backend.h"
#include "components/sessions/core/session_id.h"
#include "components/sessions/core/session_service_commands.h"
#include "components/sessions/core/session_types.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

class WorkspaceServiceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    pref_service_.registry()->RegisterDictionaryPref(kWorkspacesMetadataPref);
    service_ =
        std::make_unique<WorkspaceService>(&pref_service_, temp_dir_.GetPath());
  }

  content::BrowserTaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable pref_service_;
  base::ScopedTempDir temp_dir_;
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
  service_->SaveWorkspaceMetadata("My Workspace", /*window_count=*/2,
                                  /*tab_count=*/5, t);

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
  service_->SaveWorkspaceMetadata("Alpha", 1, 3, t1);
  service_->SaveWorkspaceMetadata("Beta", 1, 5, t2);
  service_->SaveWorkspaceMetadata("Gamma", 1, 2, t3);

  auto workspaces = service_->ListWorkspaces();
  ASSERT_EQ(workspaces.size(), 3u);
  EXPECT_EQ(workspaces[0].name, "Beta");
  EXPECT_EQ(workspaces[1].name, "Alpha");
  EXPECT_EQ(workspaces[2].name, "Gamma");
}

// Removing preference removes it from the list
TEST_F(WorkspaceServiceTest, RemoveMetadata_DisappearsFromList) {
  base::Time t = base::Time::FromSecondsSinceUnixEpoch(1700000000.0);
  service_->SaveWorkspaceMetadata("Work", 1, 3, t);
  ASSERT_EQ(service_->ListWorkspaces().size(), 1u);

  service_->RemoveWorkspaceMetadata("Work");
  EXPECT_TRUE(service_->ListWorkspaces().empty());
}

// Saving a workspace over another one replaces it
TEST_F(WorkspaceServiceTest, SaveMetadata_SameNameOverwrites) {
  base::Time t1 = base::Time::FromSecondsSinceUnixEpoch(1700000000.0);
  base::Time t2 = base::Time::FromSecondsSinceUnixEpoch(1700000100.0);
  service_->SaveWorkspaceMetadata("Work", 1, 3, t1);
  service_->SaveWorkspaceMetadata("Work", 2, 7, t2);

  auto workspaces = service_->ListWorkspaces();
  ASSERT_EQ(workspaces.size(), 1u);
  EXPECT_EQ(workspaces[0].name, "Work");
  EXPECT_EQ(workspaces[0].number_of_windows, 2);
  EXPECT_EQ(workspaces[0].number_of_tabs, 7);
}

// ---- Name / path computation tests -----------------------------------------

// Test the path sanitization (lowercases, space => dash, etc)
TEST_F(WorkspaceServiceTest, GetWorkspaceDirForName_SpacesConvertedToDashes) {
  base::FilePath path = service_->GetWorkspaceDirForName("My Workspace");
  EXPECT_EQ(path.BaseName().MaybeAsASCII(), "my-workspace");
}

// Two display names that sanitize to the same base key ("work-space") must get
// different subdirectory paths once the first is recorded in the pref.
TEST_F(WorkspaceServiceTest,
       GetWorkspaceDirForName_CollidingNames_DifferentPaths) {
  base::Time t = base::Time::FromSecondsSinceUnixEpoch(1700000000.0);
  service_->SaveWorkspaceMetadata("Work Space", 1, 3, t);

  base::FilePath path1 = service_->GetWorkspaceDirForName("Work Space");
  base::FilePath path2 = service_->GetWorkspaceDirForName("Work-Space");
  EXPECT_NE(path1, path2);

  // Version w/ space exists already; that version should be unmodified.
  EXPECT_EQ(path1.BaseName().value(), FILE_PATH_LITERAL("work-space"));

  // Version w/ dash is a new permutation. It'll have a base::PersistantHash.
  EXPECT_EQ(path2.BaseName().value(), FILE_PATH_LITERAL("work-space-46f45668"));
}

// ---- DeleteWorkspace tests --------------------------------------------------

// Deleting workspace deletes the workspace directory in profile.
TEST_F(WorkspaceServiceTest, DeleteWorkspace_RemovesDirectory) {
  base::FilePath workspace_dir = service_->GetWorkspaceDirForName("test");
  ASSERT_TRUE(base::CreateDirectory(workspace_dir));

  EXPECT_TRUE(WorkspaceService::DeleteWorkspace(workspace_dir));
  EXPECT_FALSE(base::PathExists(workspace_dir));
}

// Handle deleting a workspace that doesn't exist
TEST_F(WorkspaceServiceTest, DeleteWorkspace_NonExistent_ReturnsTrue) {
  base::FilePath nonexistent =
      temp_dir_.GetPath().AppendASCII("no-such-workspace");
  EXPECT_TRUE(WorkspaceService::DeleteWorkspace(nonexistent));
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

  base::FilePath workspace_dir = service_->GetWorkspaceDirForName("roundtrip");
  auto task_runner = base::ThreadPool::CreateSequencedTaskRunner(
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN});
  auto backend = base::MakeRefCounted<sessions::CommandStorageBackend>(
      task_runner, workspace_dir, WorkspaceService::kWorkspaceSessionType,
      /*encryptor=*/std::nullopt);

  bool error_called = false;
  bool write_done = false;
  auto on_error = base::BindPostTask(
      base::SequencedTaskRunner::GetCurrentDefault(),
      base::BindOnce([](bool* flag) { *flag = true; }, &error_called));

  task_runner->PostTask(
      FROM_HERE, base::BindOnce(&WorkspaceService::WriteWorkspaceToDisk,
                                std::move(commands), workspace_dir,
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
      task_runner, workspace_dir, WorkspaceService::kWorkspaceSessionType,
      /*encryptor=*/std::nullopt);
  base::test::TestFuture<CommandList> read_future;
  task_runner->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&WorkspaceService::ReadWorkspaceFromDisk, workspace_dir,
                     read_backend),
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
  ASSERT_TRUE(base::CreateDirectory(service_->GetWorkspacesDir()));
  base::FilePath workspace_dir = service_->GetWorkspaceDirForName("blocked");
  ASSERT_TRUE(base::WriteFile(workspace_dir, "not-a-directory"));

  auto task_runner = base::ThreadPool::CreateSequencedTaskRunner(
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN});
  auto backend = base::MakeRefCounted<sessions::CommandStorageBackend>(
      task_runner, workspace_dir, WorkspaceService::kWorkspaceSessionType,
      /*encryptor=*/std::nullopt);

  bool error_called = false;
  auto on_error = base::BindPostTask(
      base::SequencedTaskRunner::GetCurrentDefault(),
      base::BindOnce([](bool* flag) { *flag = true; }, &error_called));

  task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&WorkspaceService::WriteWorkspaceToDisk,
                     std::vector<std::unique_ptr<sessions::SessionCommand>>(),
                     workspace_dir, std::move(backend), std::move(on_error)));

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
  feature_list_.InitAndDisableFeature(features::kBraveWorkspace);
  auto* profile = profile_manager_.CreateTestingProfile("test");
  EXPECT_EQ(WorkspaceServiceFactory::GetForProfile(profile), nullptr);
}

TEST_F(WorkspaceServiceFactoryTest,
       FeatureEnabled_GetForProfileReturnsNonNull) {
  feature_list_.InitAndEnableFeature(features::kBraveWorkspace);
  auto* profile = profile_manager_.CreateTestingProfile("test");
  // ServiceIsNULLWhileTesting() suppresses service creation for test profiles.
  // Provide a testing factory so the feature-enabled path is exercised.
  WorkspaceServiceFactory::GetInstance()->SetTestingFactory(
      profile, base::BindLambdaForTesting([](content::BrowserContext* ctx)
                                              -> std::unique_ptr<KeyedService> {
        auto* p = Profile::FromBrowserContext(ctx);
        return std::make_unique<WorkspaceService>(p->GetPrefs(), p->GetPath());
      }));
  EXPECT_NE(WorkspaceServiceFactory::GetForProfile(profile), nullptr);
}
