/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/storage/snap_data_provider.h"

#include <memory>
#include <optional>
#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/string_util.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/brave_wallet/browser/snap/snap_test_utils.h"
#include "brave/components/brave_wallet/browser/snap/storage/snap_registry.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class SnapDataProviderTest : public testing::Test {
 public:
  void SetUp() override {
    SnapRegistry::RegisterProfilePrefs(prefs_.registry());
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    provider_ = std::make_unique<SnapDataProvider>(temp_dir_.GetPath(), prefs_);
  }

 protected:
  base::FilePath SnapStateFile(const std::string& snap_id) const {
    std::string dir;
    base::ReplaceChars(snap_id, ":/", "_", &dir);
    return temp_dir_.GetPath()
        .Append(FILE_PATH_LITERAL("BraveWallet"))
        .Append(FILE_PATH_LITERAL("Snaps"))
        .AppendASCII(dir)
        .Append(FILE_PATH_LITERAL("state.json"));
  }

  // Writes state and asserts no error was returned.
  void UpdateState(const std::string& snap_id, const std::string& json) {
    base::test::TestFuture<std::optional<std::string>> future;
    provider_->UpdateSnapState(snap_id, json, future.GetCallback());
    EXPECT_FALSE(future.Get().has_value()) << *future.Get();
  }

  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  TestingPrefServiceSimple prefs_;
  std::unique_ptr<SnapDataProvider> provider_;
};

TEST_F(SnapDataProviderTest, RegistryPassthroughAfterInstall) {
  const std::string snap_id = "npm:@test/snap";
  EXPECT_FALSE(provider_->IsInstalled(snap_id));
  EXPECT_FALSE(provider_->GetSnap(snap_id));
  EXPECT_TRUE(provider_->GetAllSnaps().empty());

  provider_->OnSnapInstalled(*MakeTestSnapInstallData(snap_id, "1.2.3"));

  EXPECT_TRUE(provider_->IsInstalled(snap_id));
  auto snap = provider_->GetSnap(snap_id);
  ASSERT_TRUE(snap);
  EXPECT_EQ(snap->snap_id, snap_id);
  EXPECT_EQ(snap->version, "1.2.3");
  ASSERT_TRUE(snap->manifest);
  EXPECT_EQ(provider_->GetAllSnaps().size(), 1u);
}

TEST_F(SnapDataProviderTest, GetSnapReturnsIndependentClone) {
  const std::string snap_id = "npm:@test/snap";
  provider_->OnSnapInstalled(*MakeTestSnapInstallData(snap_id, "1.0.0"));

  auto first = provider_->GetSnap(snap_id);
  ASSERT_TRUE(first);
  first->version = "mutated";

  auto second = provider_->GetSnap(snap_id);
  ASSERT_TRUE(second);
  EXPECT_EQ(second->version, "1.0.0");
}

TEST_F(SnapDataProviderTest, SaveBundleFromDirThenReadBundle) {
  const std::string snap_id = "npm:@test/snap";
  base::FilePath unpacked;
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    unpacked = temp_dir_.GetPath().AppendASCII("unpacked");
    ASSERT_TRUE(base::CreateDirectory(unpacked));
    ASSERT_TRUE(base::WriteFile(unpacked.AppendASCII("bundle.js"), "BUNDLE"));
    ASSERT_TRUE(base::WriteFile(unpacked.AppendASCII("manifest.json"), "{}"));
  }

  base::test::TestFuture<bool> save;
  provider_->SaveBundleFromDir(snap_id, unpacked, save.GetCallback());
  EXPECT_TRUE(save.Get());

  base::test::TestFuture<std::optional<std::string>> read;
  provider_->ReadBundle(snap_id, read.GetCallback());
  auto bundle = read.Get();
  ASSERT_TRUE(bundle);
  EXPECT_EQ(*bundle, "BUNDLE");
}

TEST_F(SnapDataProviderTest, UpdateThenGetSnapState) {
  const std::string snap_id = "npm:@test/snap";
  UpdateState(snap_id, R"({"counter":1})");

  base::test::TestFuture<std::optional<std::string>> get;
  provider_->GetSnapState(snap_id, get.GetCallback());
  auto state = get.Get();
  ASSERT_TRUE(state);
  EXPECT_EQ(*state, R"({"counter":1})");
}

TEST_F(SnapDataProviderTest, GetSnapStateServedFromCache) {
  const std::string snap_id = "npm:@test/snap";
  UpdateState(snap_id, R"({"counter":1})");

  // Delete the on-disk copy; a cache hit must not touch disk.
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(base::DeleteFile(SnapStateFile(snap_id)));
  }

  base::test::TestFuture<std::optional<std::string>> get;
  provider_->GetSnapState(snap_id, get.GetCallback());
  auto state = get.Get();
  ASSERT_TRUE(state);
  EXPECT_EQ(*state, R"({"counter":1})");
}

TEST_F(SnapDataProviderTest, GetSnapStateReadsFromDiskWhenNotCached) {
  const std::string snap_id = "npm:@test/snap";
  UpdateState(snap_id, R"({"counter":7})");

  // A fresh provider has an empty in-session cache and must read from disk.
  SnapDataProvider fresh(temp_dir_.GetPath(), prefs_);
  base::test::TestFuture<std::optional<std::string>> get;
  fresh.GetSnapState(snap_id, get.GetCallback());
  auto state = get.Get();
  ASSERT_TRUE(state);
  EXPECT_EQ(*state, R"({"counter":7})");
}

TEST_F(SnapDataProviderTest, UpdateSnapStateRejectsOversized) {
  const std::string snap_id = "npm:@test/snap";
  std::string oversized(1024 * 1024 + 1, 'x');

  base::test::TestFuture<std::optional<std::string>> update;
  provider_->UpdateSnapState(snap_id, oversized, update.GetCallback());
  auto error = update.Get();
  ASSERT_TRUE(error);
  EXPECT_EQ(*error, "snap_manageState: state exceeds 1 MB limit");

  // Nothing was written.
  base::test::TestFuture<std::optional<std::string>> get;
  provider_->GetSnapState(snap_id, get.GetCallback());
  EXPECT_FALSE(get.Get().has_value());
}

TEST_F(SnapDataProviderTest, ClearSnapStateEvictsCache) {
  const std::string snap_id = "npm:@test/snap";
  UpdateState(snap_id, R"({"counter":1})");

  base::test::TestFuture<std::optional<std::string>> clear;
  provider_->ClearSnapState(snap_id, clear.GetCallback());
  EXPECT_FALSE(clear.Get().has_value());

  // Cache now holds the empty sentinel → reads report no state.
  base::test::TestFuture<std::optional<std::string>> get;
  provider_->GetSnapState(snap_id, get.GetCallback());
  EXPECT_FALSE(get.Get().has_value());
}

TEST_F(SnapDataProviderTest, OnSnapUninstalledRemovesRegistryStateAndFiles) {
  const std::string snap_id = "npm:@test/snap";
  provider_->OnSnapInstalled(*MakeTestSnapInstallData(snap_id, "1.0.0"));
  UpdateState(snap_id, R"({"x":1})");
  ASSERT_TRUE(provider_->IsInstalled(snap_id));
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(base::PathExists(SnapStateFile(snap_id)));
  }

  provider_->OnSnapUninstalled(snap_id);

  EXPECT_FALSE(provider_->IsInstalled(snap_id));
  EXPECT_FALSE(provider_->GetSnap(snap_id));

  // The bundle directory is deleted asynchronously (fire-and-forget pool task).
  task_environment_.RunUntilIdle();
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    EXPECT_FALSE(base::PathExists(SnapStateFile(snap_id)));
  }

  // Cache was evicted: the read falls through to the now-deleted disk file.
  base::test::TestFuture<std::optional<std::string>> get;
  provider_->GetSnapState(snap_id, get.GetCallback());
  EXPECT_FALSE(get.Get().has_value());
}

}  // namespace brave_wallet
