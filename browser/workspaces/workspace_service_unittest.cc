/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/workspaces/workspace_service.h"

#include <memory>
#include <string>
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
#include "base/uuid.h"
#include "base/values.h"
#include "brave/browser/workspaces/features.h"
#include "brave/browser/workspaces/pref_names.h"
#include "brave/browser/workspaces/workspace_metadata.h"
#include "brave/browser/workspaces/workspace_service_factory.h"
#include "brave/browser/workspaces/workspace_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/prefs/pref_service.h"
#include "components/sessions/core/command_storage_backend.h"
#include "components/sessions/core/session_id.h"
#include "components/sessions/core/session_service_commands.h"
#include "components/sessions/core/session_types.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
WorkspaceMetadata MakeMeta(const std::string& name, int tabs, base::Time t) {
  return {.id = base::Uuid::GenerateRandomV4(),
          .name = name,
          .modified_at = t,
          .number_of_tabs = tabs};
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
  WorkspaceMetadata meta = MakeMeta("My Workspace", 5, t);
  service_->SaveWorkspaceMetadata(meta);

  auto workspaces = service_->ListWorkspaces();
  ASSERT_EQ(workspaces.size(), 1u);
  EXPECT_EQ(workspaces[0].id, meta.id);
  EXPECT_EQ(workspaces[0].name, "My Workspace");
  EXPECT_EQ(workspaces[0].number_of_tabs, 5);
  EXPECT_EQ(workspaces[0].modified_at, t);
}

// Verify listing of workspaces orders by date modified desc
TEST_F(WorkspaceServiceTest, ListWorkspaces_SortedByModifiedAtDescending) {
  base::Time t1 = base::Time::FromSecondsSinceUnixEpoch(1700000000.0);
  base::Time t2 = base::Time::FromSecondsSinceUnixEpoch(1700000100.0);
  base::Time t3 = base::Time::FromSecondsSinceUnixEpoch(1699999900.0);
  service_->SaveWorkspaceMetadata(MakeMeta("Alpha", 3, t1));
  service_->SaveWorkspaceMetadata(MakeMeta("Beta", 5, t2));
  service_->SaveWorkspaceMetadata(MakeMeta("Gamma", 2, t3));

  auto workspaces = service_->ListWorkspaces();
  ASSERT_EQ(workspaces.size(), 3u);
  EXPECT_EQ(workspaces[0].name, "Beta");
  EXPECT_EQ(workspaces[1].name, "Alpha");
  EXPECT_EQ(workspaces[2].name, "Gamma");
}

// Removing preference removes it from the list
TEST_F(WorkspaceServiceTest, RemoveMetadata_DisappearsFromList) {
  base::Time t = base::Time::FromSecondsSinceUnixEpoch(1700000000.0);
  WorkspaceMetadata meta = MakeMeta("Work", 3, t);
  service_->SaveWorkspaceMetadata(meta);
  ASSERT_EQ(service_->ListWorkspaces().size(), 1u);

  service_->RemoveWorkspaceMetadata(meta.id);
  EXPECT_TRUE(service_->ListWorkspaces().empty());
}

// Saving over the same identity replaces that entry.
TEST_F(WorkspaceServiceTest, SaveMetadata_SameIdOverwrites) {
  base::Time t1 = base::Time::FromSecondsSinceUnixEpoch(1700000000.0);
  base::Time t2 = base::Time::FromSecondsSinceUnixEpoch(1700000100.0);
  WorkspaceMetadata meta = MakeMeta("Work", 3, t1);
  service_->SaveWorkspaceMetadata(meta);

  meta.number_of_tabs = 7;
  meta.modified_at = t2;
  service_->SaveWorkspaceMetadata(meta);

  auto workspaces = service_->ListWorkspaces();
  ASSERT_EQ(workspaces.size(), 1u);
  EXPECT_EQ(workspaces[0].name, "Work");
  EXPECT_EQ(workspaces[0].number_of_tabs, 7);
}

// Identity is a UUID, so two workspaces may share a display name.
TEST_F(WorkspaceServiceTest, SaveMetadata_SameNameDistinctIdsCoexist) {
  base::Time t = base::Time::FromSecondsSinceUnixEpoch(1700000000.0);
  service_->SaveWorkspaceMetadata(MakeMeta("Work", 3, t));
  service_->SaveWorkspaceMetadata(MakeMeta("Work", 7, t));

  EXPECT_EQ(service_->ListWorkspaces().size(), 2u);
}

// Entries whose key is not a UUID are ignored, which is how metadata written
// before workspaces were UUID-keyed is discarded.
TEST_F(WorkspaceServiceTest, ListWorkspaces_SkipsNonUuidKeys) {
  base::DictValue dict;
  dict.Set("deadbeef",
           base::DictValue().Set("name", "Legacy").Set("number-of-tabs", 4));
  profile_->GetPrefs()->SetDict(kWorkspacesMetadataPref, std::move(dict));

  EXPECT_TRUE(service_->ListWorkspaces().empty());
}

// ---- Create / rename / lookup ----------------------------------------------

// A newly created workspace is listed and has no tabs yet.
TEST_F(WorkspaceServiceTest, CreateWorkspace_IsEmptyAndListed) {
  WorkspaceMetadata created = service_->CreateWorkspace("Research");

  EXPECT_TRUE(created.id.is_valid());
  EXPECT_EQ(created.number_of_tabs, 0);

  auto workspaces = service_->ListWorkspaces();
  ASSERT_EQ(workspaces.size(), 1u);
  EXPECT_EQ(workspaces[0].id, created.id);
  EXPECT_EQ(workspaces[0].name, "Research");
  EXPECT_EQ(workspaces[0].number_of_tabs, 0);
}

TEST_F(WorkspaceServiceTest, CreateWorkspace_SameNameGetsDistinctIds) {
  WorkspaceMetadata first = service_->CreateWorkspace("Work");
  WorkspaceMetadata second = service_->CreateWorkspace("Work");

  EXPECT_NE(first.id, second.id);
  EXPECT_EQ(service_->ListWorkspaces().size(), 2u);
}

TEST_F(WorkspaceServiceTest, GetWorkspace_UnknownIdReturnsNullopt) {
  service_->CreateWorkspace("Work");
  EXPECT_FALSE(
      service_->GetWorkspace(base::Uuid::GenerateRandomV4()).has_value());
}

// Renaming keeps identity, so the on-disk location does not move.
TEST_F(WorkspaceServiceTest, RenameWorkspace_KeepsIdentityAndPath) {
  WorkspaceMetadata created = service_->CreateWorkspace("Work");
  base::FilePath path_before = service_->GetWorkspacePathForId(created.id);

  service_->RenameWorkspace(created.id, "Research");

  auto renamed = service_->GetWorkspace(created.id);
  ASSERT_TRUE(renamed.has_value());
  EXPECT_EQ(renamed->name, "Research");
  EXPECT_EQ(service_->GetWorkspacePathForId(created.id), path_before);
}

TEST_F(WorkspaceServiceTest, RenameWorkspace_EmptyNameIsIgnored) {
  WorkspaceMetadata created = service_->CreateWorkspace("Work");

  service_->RenameWorkspace(created.id, "");

  auto workspace = service_->GetWorkspace(created.id);
  ASSERT_TRUE(workspace.has_value());
  EXPECT_EQ(workspace->name, "Work");
}

TEST_F(WorkspaceServiceTest, RenameWorkspace_UnknownIdIsNoOp) {
  service_->RenameWorkspace(base::Uuid::GenerateRandomV4(), "Research");
  EXPECT_TRUE(service_->ListWorkspaces().empty());
}

// ---- Window binding ---------------------------------------------------------

TEST_F(WorkspaceServiceTest, Binding_UnloadedWorkspaceHasNoWindow) {
  WorkspaceMetadata created = service_->CreateWorkspace("Work");
  EXPECT_FALSE(service_->GetWindowForWorkspace(created.id).has_value());
}

TEST_F(WorkspaceServiceTest, Binding_UnboundWindowHostsNothing) {
  EXPECT_FALSE(
      service_->GetWorkspaceForWindow(SessionID::NewUnique()).has_value());
}

TEST_F(WorkspaceServiceTest, Binding_UnbindUnknownWindowIsSafe) {
  service_->UnbindWindow(SessionID::NewUnique());
  SUCCEED();
}

// A window nobody switched away from has no unmanaged window behind it.
TEST_F(WorkspaceServiceTest, Unmanaged_WindowHasNoOriginUntilSwitched) {
  EXPECT_FALSE(
      service_->GetUnmanagedWindowFor(SessionID::NewUnique()).has_value());
}

// ---- Name / path computation tests -----------------------------------------

TEST_F(WorkspaceServiceTest, GetWorkspacePathForId_DifferentIdsDifferentPaths) {
  EXPECT_NE(service_->GetWorkspacePathForId(base::Uuid::GenerateRandomV4()),
            service_->GetWorkspacePathForId(base::Uuid::GenerateRandomV4()));
}

TEST_F(WorkspaceServiceTest, GetWorkspacePathForId_SameIdSamePath) {
  base::Uuid id = base::Uuid::GenerateRandomV4();
  EXPECT_EQ(service_->GetWorkspacePathForId(id),
            service_->GetWorkspacePathForId(id));
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
      service_->GetWorkspacePathForId(base::Uuid::GenerateRandomV4());
  auto task_runner = base::ThreadPool::CreateSequencedTaskRunner(
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN});
  auto backend = base::MakeRefCounted<sessions::CommandStorageBackend>(
      task_runner, workspace_path, kWorkspaceSessionType,
      /*encryptor=*/nullptr);

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

  // Read back using a fresh backend (mirrors SwitchWindowToWorkspace's
  // pattern).
  auto read_backend = base::MakeRefCounted<sessions::CommandStorageBackend>(
      task_runner, workspace_path, kWorkspaceSessionType,
      /*encryptor=*/nullptr);
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
  EXPECT_EQ(read_cmds[0]->Serialize(/*encryptor*/ nullptr),
            sessions::CreateSetWindowTypeCommand(
                window_id, sessions::SessionWindow::TYPE_NORMAL)
                ->Serialize(/*encryptor*/ nullptr));
  EXPECT_EQ(read_cmds[1]->Serialize(/*encryptor*/ nullptr),
            sessions::CreateSetSelectedTabInWindowCommand(window_id, 0)
                ->Serialize(/*encryptor*/ nullptr));
}

TEST_F(WorkspaceServiceTest,
       WriteToDisk_DirectoryCreationFailure_CallsOnError) {
  // Place a regular FILE where the workspace directory would be so that
  // base::CreateDirectory fails on the background thread and on_error is
  // posted back to the UI thread.
  ASSERT_TRUE(base::CreateDirectory(service_->GetWorkspacesPath()));
  base::FilePath workspace_path =
      service_->GetWorkspacePathForId(base::Uuid::GenerateRandomV4());
  ASSERT_TRUE(base::WriteFile(workspace_path, "not-a-directory"));

  auto task_runner = base::ThreadPool::CreateSequencedTaskRunner(
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN});
  auto backend = base::MakeRefCounted<sessions::CommandStorageBackend>(
      task_runner, workspace_path, kWorkspaceSessionType,
      /*encryptor=*/nullptr);

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
