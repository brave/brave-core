/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/storage/snap_storage.h"

#include <optional>
#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/string_util.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/threading/sequence_bound.h"
#include "base/threading/thread_restrictions.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class SnapStorageTest : public testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    snaps_dir_ = temp_dir_.GetPath()
                     .Append(FILE_PATH_LITERAL("BraveWallet"))
                     .Append(FILE_PATH_LITERAL("Snaps"));
    storage_ = base::SequenceBound<SnapStorage>(
        base::ThreadPool::CreateSequencedTaskRunner(
            {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
             base::TaskShutdownBehavior::BLOCK_SHUTDOWN}),
        snaps_dir_, base::SequencedTaskRunner::GetCurrentDefault());
  }

 protected:
  // Mirrors SnapStorage::GetSnapDir for on-disk assertions.
  base::FilePath SnapDir(const std::string& snap_id) const {
    std::string dir;
    base::ReplaceChars(snap_id, ":/", "_", &dir);
    return snaps_dir_.AppendASCII(dir);
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

  bool MoveSnapFiles(const std::string& snap_id,
                     const base::FilePath& unpacked) {
    base::test::TestFuture<bool> future;
    storage_.AsyncCall(&SnapStorage::MoveSnapFiles)
        .WithArgs(snap_id, unpacked)
        .Then(future.GetCallback());
    return future.Get();
  }

  std::optional<std::string> ReadBundle(const std::string& snap_id) {
    base::test::TestFuture<std::optional<std::string>> future;
    storage_.AsyncCall(&SnapStorage::ReadBundle)
        .WithArgs(snap_id)
        .Then(future.GetCallback());
    return future.Get();
  }

  bool WriteState(const std::string& snap_id, const std::string& state) {
    base::test::TestFuture<bool> future;
    storage_.AsyncCall(&SnapStorage::WriteState)
        .WithArgs(snap_id, state)
        .Then(future.GetCallback());
    return future.Get();
  }

  std::optional<std::string> ReadState(const std::string& snap_id) {
    base::test::TestFuture<std::optional<std::string>> future;
    storage_.AsyncCall(&SnapStorage::ReadState)
        .WithArgs(snap_id)
        .Then(future.GetCallback());
    return future.Get();
  }

  bool DeleteSnap(const std::string& snap_id) {
    base::test::TestFuture<bool> future;
    storage_.AsyncCall(&SnapStorage::DeleteSnap)
        .WithArgs(snap_id)
        .Then(future.GetCallback());
    return future.Get();
  }

  bool HasSnap(const std::string& snap_id) {
    base::test::TestFuture<bool> future;
    storage_.AsyncCall(&SnapStorage::HasSnap)
        .WithArgs(snap_id)
        .Then(future.GetCallback());
    return future.Get();
  }

  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  base::FilePath snaps_dir_;
  base::SequenceBound<SnapStorage> storage_;
};

TEST_F(SnapStorageTest, MoveSnapFilesMovesBundleAndManifest) {
  const std::string snap_id = "npm:@test/snap";
  base::FilePath unpacked = MakeUnpackedDir("BUNDLE_JS", "MANIFEST_JSON");

  EXPECT_TRUE(MoveSnapFiles(snap_id, unpacked));

  auto bundle = ReadBundle(snap_id);
  ASSERT_TRUE(bundle);
  EXPECT_EQ(*bundle, "BUNDLE_JS");

  base::ScopedAllowBlockingForTesting allow_blocking;
  EXPECT_TRUE(base::PathExists(
      SnapDir(snap_id).Append(FILE_PATH_LITERAL("manifest.json"))));
  // The files were moved, not copied.
  EXPECT_FALSE(base::PathExists(unpacked.AppendASCII("bundle.js")));
}

TEST_F(SnapStorageTest, MoveSnapFilesMissingSourceReturnsFalse) {
  base::FilePath nonexistent =
      temp_dir_.GetPath().AppendASCII("does_not_exist");
  EXPECT_FALSE(MoveSnapFiles("npm:@test/snap", nonexistent));
}

TEST_F(SnapStorageTest, ReadBundleMissingReturnsNullopt) {
  EXPECT_FALSE(ReadBundle("npm:@nope/snap").has_value());
}

TEST_F(SnapStorageTest, WriteStateThenReadState) {
  const std::string snap_id = "npm:@test/snap";
  EXPECT_TRUE(WriteState(snap_id, R"({"counter":1})"));

  auto state = ReadState(snap_id);
  ASSERT_TRUE(state);
  EXPECT_EQ(*state, R"({"counter":1})");
}

TEST_F(SnapStorageTest, ReadStateMissingReturnsNullopt) {
  EXPECT_FALSE(ReadState("npm:@nope/snap").has_value());
}

TEST_F(SnapStorageTest, DeleteSnapRemovesDirectory) {
  const std::string snap_id = "npm:@test/snap";
  EXPECT_TRUE(WriteState(snap_id, "{}"));
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(base::PathExists(SnapDir(snap_id)));
  }

  EXPECT_TRUE(DeleteSnap(snap_id));

  base::ScopedAllowBlockingForTesting allow_blocking;
  EXPECT_FALSE(base::PathExists(SnapDir(snap_id)));
}

TEST_F(SnapStorageTest, HasSnapReflectsBundlePresence) {
  const std::string snap_id = "npm:@test/snap";
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    EXPECT_FALSE(HasSnap(snap_id));
  }

  EXPECT_TRUE(MoveSnapFiles(snap_id, MakeUnpackedDir("B", "M")));

  base::ScopedAllowBlockingForTesting allow_blocking;
  EXPECT_TRUE(HasSnap(snap_id));
}

TEST_F(SnapStorageTest, SnapIdCharactersAreSanitized) {
  // ':' and '/' become '_'; other characters (like '-') are preserved.
  const std::string snap_id = "npm:@scope/cool-snap";
  EXPECT_TRUE(WriteState(snap_id, "{}"));

  base::ScopedAllowBlockingForTesting allow_blocking;
  base::FilePath snaps_root = temp_dir_.GetPath()
                                  .Append(FILE_PATH_LITERAL("BraveWallet"))
                                  .Append(FILE_PATH_LITERAL("Snaps"));
  EXPECT_TRUE(base::PathExists(snaps_root.AppendASCII("npm_@scope_cool-snap")));
}

}  // namespace brave_wallet
