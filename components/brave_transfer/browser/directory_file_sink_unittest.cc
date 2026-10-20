// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/browser/directory_file_sink.h"

#include <memory>
#include <optional>
#include <vector>

#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/synchronization/lock.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_transfer {

class BraveTransferDirectoryFileSinkTest : public testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    preferred_ = temp_dir_.GetPath().AppendASCII("chosen");
    fallback_ = temp_dir_.GetPath().AppendASCII("Downloads");
    sink_ = std::make_unique<DirectoryFileSink>(
        base::BindLambdaForTesting([this] {
          return DirectoryFileSink::Directories{preferred_, fallback_};
        }),
        base::BindLambdaForTesting([this](const base::FilePath& path) {
          base::AutoLock lock(lock_);
          finalized_.push_back(path);
        }));
  }

  std::optional<SavedFile> Commit(const std::string& key) {
    base::test::TestFuture<std::optional<SavedFile>> future;
    sink_->Commit(key, future.GetCallback());
    return future.Take();
  }

  std::vector<base::FilePath> Finalized() {
    base::AutoLock lock(lock_);
    return finalized_;
  }

  int CountFiles(const base::FilePath& directory) {
    int count = 0;
    base::FileEnumerator files(directory, false, base::FileEnumerator::FILES);
    for (base::FilePath path = files.Next(); !path.empty();
         path = files.Next()) {
      ++count;
    }
    return count;
  }

  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  base::FilePath preferred_;
  base::FilePath fallback_;
  base::Lock lock_;
  std::vector<base::FilePath> finalized_;
  std::unique_ptr<DirectoryFileSink> sink_;
};

TEST_F(BraveTransferDirectoryFileSinkTest, WritesToPreferredFolder) {
  bool failed = false;
  sink_->Begin("a", "photo.jpg",
               base::BindLambdaForTesting([&] { failed = true; }));
  sink_->Append("a", "hello ");
  sink_->Append("a", "world");
  std::optional<SavedFile> saved = Commit("a");

  ASSERT_TRUE(saved);
  EXPECT_FALSE(failed);
  EXPECT_EQ(saved->path, preferred_.AppendASCII("photo.jpg"));
  EXPECT_FALSE(saved->used_fallback);
  std::string contents;
  ASSERT_TRUE(base::ReadFileToString(saved->path, &contents));
  EXPECT_EQ(contents, "hello world");
  EXPECT_EQ(CountFiles(preferred_), 1);
  EXPECT_EQ(Finalized(), std::vector<base::FilePath>{saved->path});
}

TEST_F(BraveTransferDirectoryFileSinkTest, UniqueNames) {
  ASSERT_TRUE(base::CreateDirectory(preferred_));
  ASSERT_TRUE(base::WriteFile(preferred_.AppendASCII("a.txt"), "old"));
  sink_->Begin("1", "a.txt", base::DoNothing());
  sink_->Begin("2", "a.txt", base::DoNothing());
  sink_->Append("1", "one");
  sink_->Append("2", "two");
  std::optional<SavedFile> first = Commit("1");
  std::optional<SavedFile> second = Commit("2");
  ASSERT_TRUE(first);
  ASSERT_TRUE(second);
  EXPECT_NE(first->path, second->path);
  EXPECT_NE(first->path, preferred_.AppendASCII("a.txt"));
  std::string contents;
  ASSERT_TRUE(
      base::ReadFileToString(preferred_.AppendASCII("a.txt"), &contents));
  EXPECT_EQ(contents, "old");
  EXPECT_EQ(CountFiles(preferred_), 3);
}

TEST_F(BraveTransferDirectoryFileSinkTest, FallsBackWhenFolderUnusable) {
  ASSERT_TRUE(base::WriteFile(preferred_, "not a folder"));
  sink_->Begin("a", "doc.pdf", base::DoNothing());
  sink_->Append("a", "pdf");
  std::optional<SavedFile> saved = Commit("a");
  ASSERT_TRUE(saved);
  EXPECT_EQ(saved->path, fallback_.AppendASCII("doc.pdf"));
  EXPECT_TRUE(saved->used_fallback);
}

TEST_F(BraveTransferDirectoryFileSinkTest, EmptyPreferredUsesDefault) {
  preferred_ = base::FilePath();
  sink_->Begin("a", "doc.pdf", base::DoNothing());
  std::optional<SavedFile> saved = Commit("a");
  ASSERT_TRUE(saved);
  EXPECT_EQ(saved->path, fallback_.AppendASCII("doc.pdf"));
  EXPECT_FALSE(saved->used_fallback);
  EXPECT_EQ(base::GetFileSize(saved->path), 0);
}

TEST_F(BraveTransferDirectoryFileSinkTest, AbortDeletesPartialFile) {
  sink_->Begin("a", "big.bin", base::DoNothing());
  sink_->Append("a", "partial");
  sink_->Abort("a");
  task_environment_.RunUntilIdle();
  EXPECT_EQ(CountFiles(preferred_), 0);
  EXPECT_FALSE(Commit("a"));
  EXPECT_TRUE(Finalized().empty());
}

TEST_F(BraveTransferDirectoryFileSinkTest, ReportsFailureOnce) {
  ASSERT_TRUE(base::WriteFile(preferred_, "not a folder"));
  ASSERT_TRUE(base::WriteFile(fallback_, "not a folder either"));
  int failures = 0;
  sink_->Begin("a", "x", base::BindLambdaForTesting([&] { ++failures; }));
  sink_->Append("a", "1");
  sink_->Append("a", "2");
  task_environment_.RunUntilIdle();
  EXPECT_EQ(failures, 1);
  EXPECT_FALSE(Commit("a"));
}

TEST_F(BraveTransferDirectoryFileSinkTest, DestroyingRemovesPartialFiles) {
  sink_->Begin("a", "big.bin", base::DoNothing());
  sink_->Append("a", "partial");
  task_environment_.RunUntilIdle();
  EXPECT_EQ(CountFiles(preferred_), 1);
  sink_.reset();
  task_environment_.RunUntilIdle();
  EXPECT_EQ(CountFiles(preferred_), 0);
}

}  // namespace brave_transfer
