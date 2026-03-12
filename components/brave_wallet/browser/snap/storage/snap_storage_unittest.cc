/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/storage/snap_storage.h"

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
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class SnapStorageTest : public testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    storage_ = std::make_unique<SnapStorage>(temp_dir_.GetPath());
  }

 protected:
  // Mirrors SnapStorage::GetSnapDir for on-disk assertions.
  base::FilePath SnapDir(const std::string& snap_id) const {
    std::string dir;
    base::ReplaceChars(snap_id, ":/", "_", &dir);
    return temp_dir_.GetPath()
        .Append(FILE_PATH_LITERAL("BraveWallet"))
        .Append(FILE_PATH_LITERAL("Snaps"))
        .AppendASCII(dir);
  }

  base::FilePath MakeUnpackedDir(const std::string& bundle,
                                 const std::string& manifest) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    base::FilePath dir = temp_dir_.GetPath().AppendASCII("unpacked");
    CHECK(base::CreateDirectory(dir));
    CHECK(base::WriteFile(dir.AppendASCII("bundle.js"), bundle));
    CHECK(base::WriteFile(dir.AppendASCII("manifest.json"), manifest));
    return dir;
  }

  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  std::unique_ptr<SnapStorage> storage_;
};

TEST_F(SnapStorageTest, MoveSnapFilesMovesBundleAndManifest) {
  const std::string snap_id = "npm:@test/snap";
  base::FilePath unpacked = MakeUnpackedDir("BUNDLE_JS", "MANIFEST_JSON");

  base::test::TestFuture<bool> move;
  storage_->MoveSnapFiles(snap_id, unpacked, move.GetCallback());
  EXPECT_TRUE(move.Get());

  base::test::TestFuture<std::optional<std::string>> read;
  storage_->ReadBundle(snap_id, read.GetCallback());
  auto bundle = read.Get();
  ASSERT_TRUE(bundle);
  EXPECT_EQ(*bundle, "BUNDLE_JS");

  base::ScopedAllowBlockingForTesting allow_blocking;
  EXPECT_TRUE(base::PathExists(
      SnapDir(snap_id).Append(FILE_PATH_LITERAL("manifest.json"))));
  // The files were moved, not copied.
  EXPECT_FALSE(base::PathExists(unpacked.AppendASCII("bundle.js")));
}

TEST_F(SnapStorageTest, MoveSnapFilesMissingSourceReturnsFalse) {
  base::FilePath nonexistent = temp_dir_.GetPath().AppendASCII("does_not_exist");
  base::test::TestFuture<bool> move;
  storage_->MoveSnapFiles("npm:@test/snap", nonexistent, move.GetCallback());
  EXPECT_FALSE(move.Get());
}

TEST_F(SnapStorageTest, ReadBundleMissingReturnsNullopt) {
  base::test::TestFuture<std::optional<std::string>> read;
  storage_->ReadBundle("npm:@nope/snap", read.GetCallback());
  EXPECT_FALSE(read.Get().has_value());
}

TEST_F(SnapStorageTest, WriteStateThenReadState) {
  const std::string snap_id = "npm:@test/snap";
  base::test::TestFuture<bool> write;
  storage_->WriteState(snap_id, R"({"counter":1})", write.GetCallback());
  EXPECT_TRUE(write.Get());

  base::test::TestFuture<std::optional<std::string>> read;
  storage_->ReadState(snap_id, read.GetCallback());
  auto state = read.Get();
  ASSERT_TRUE(state);
  EXPECT_EQ(*state, R"({"counter":1})");
}

TEST_F(SnapStorageTest, WriteStateCreatesDirectoryForBuiltInSnaps) {
  // A built-in snap has no bundle directory; WriteState must create it.
  const std::string snap_id = "npm:builtin-snap";
  base::test::TestFuture<bool> write;
  storage_->WriteState(snap_id, R"({"k":1})", write.GetCallback());
  EXPECT_TRUE(write.Get());

  base::ScopedAllowBlockingForTesting allow_blocking;
  EXPECT_TRUE(base::PathExists(
      SnapDir(snap_id).Append(FILE_PATH_LITERAL("state.json"))));
}

TEST_F(SnapStorageTest, ReadStateMissingReturnsNullopt) {
  base::test::TestFuture<std::optional<std::string>> read;
  storage_->ReadState("npm:@nope/snap", read.GetCallback());
  EXPECT_FALSE(read.Get().has_value());
}

TEST_F(SnapStorageTest, DeleteSnapRemovesDirectory) {
  const std::string snap_id = "npm:@test/snap";
  base::test::TestFuture<bool> write;
  storage_->WriteState(snap_id, "{}", write.GetCallback());
  EXPECT_TRUE(write.Get());
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(base::PathExists(SnapDir(snap_id)));
  }

  storage_->DeleteSnap(snap_id);  // Fire-and-forget BEST_EFFORT pool task.
  task_environment_.RunUntilIdle();

  base::ScopedAllowBlockingForTesting allow_blocking;
  EXPECT_FALSE(base::PathExists(SnapDir(snap_id)));
}

TEST_F(SnapStorageTest, HasSnapReflectsBundlePresence) {
  const std::string snap_id = "npm:@test/snap";
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    EXPECT_FALSE(storage_->HasSnap(snap_id));
  }

  base::test::TestFuture<bool> move;
  storage_->MoveSnapFiles(snap_id, MakeUnpackedDir("B", "M"), move.GetCallback());
  EXPECT_TRUE(move.Get());

  base::ScopedAllowBlockingForTesting allow_blocking;
  EXPECT_TRUE(storage_->HasSnap(snap_id));
}

TEST_F(SnapStorageTest, SnapIdCharactersAreSanitized) {
  // ':' and '/' become '_'; other characters (like '-') are preserved.
  const std::string snap_id = "npm:@scope/cool-snap";
  base::test::TestFuture<bool> write;
  storage_->WriteState(snap_id, "{}", write.GetCallback());
  EXPECT_TRUE(write.Get());

  base::ScopedAllowBlockingForTesting allow_blocking;
  base::FilePath snaps_root = temp_dir_.GetPath()
                                  .Append(FILE_PATH_LITERAL("BraveWallet"))
                                  .Append(FILE_PATH_LITERAL("Snaps"));
  EXPECT_TRUE(base::PathExists(snaps_root.AppendASCII("npm_@scope_cool-snap")));
}

}  // namespace brave_wallet
