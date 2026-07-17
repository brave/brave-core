// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/workspace/workspace_service.h"

#include <optional>
#include <string>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class WorkspaceServiceTest : public testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    ASSERT_TRUE(base::WriteFile(temp_dir_.GetPath().AppendASCII("a.txt"),
                                "hello\nworld\n"));
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  WorkspaceService service_;
};

TEST_F(WorkspaceServiceTest, RootManagement) {
  const std::string conv = "conv-1";
  EXPECT_FALSE(service_.HasWorkspace(conv));
  EXPECT_FALSE(service_.GetWorkspaceRoot(conv));

  service_.SetWorkspaceRoot(conv, temp_dir_.GetPath());
  EXPECT_TRUE(service_.HasWorkspace(conv));
  EXPECT_EQ(service_.GetWorkspaceRoot(conv), temp_dir_.GetPath());

  service_.ClearWorkspace(conv);
  EXPECT_FALSE(service_.HasWorkspace(conv));
}

TEST_F(WorkspaceServiceTest, ReadFileNoWorkspaceReturnsError) {
  base::test::TestFuture<workspace::FileOpResult> future;
  service_.ReadFile("missing-conv", "a.txt", std::nullopt, std::nullopt,
                    future.GetCallback());
  EXPECT_FALSE(future.Take().has_value());
}

TEST_F(WorkspaceServiceTest, ReadFileReturnsContent) {
  const std::string conv = "conv-1";
  service_.SetWorkspaceRoot(conv, temp_dir_.GetPath());

  base::test::TestFuture<workspace::FileOpResult> future;
  service_.ReadFile(conv, "a.txt", std::nullopt, std::nullopt,
                    future.GetCallback());
  workspace::FileOpResult result = future.Take();
  ASSERT_TRUE(result.has_value());
  EXPECT_NE(result->find("hello"), std::string::npos);
}

TEST_F(WorkspaceServiceTest, ReadFileConfinedToRoot) {
  const std::string conv = "conv-1";
  service_.SetWorkspaceRoot(conv, temp_dir_.GetPath());

  base::test::TestFuture<workspace::FileOpResult> future;
  service_.ReadFile(conv, "../escape.txt", std::nullopt, std::nullopt,
                    future.GetCallback());
  EXPECT_FALSE(future.Take().has_value());
}

TEST_F(WorkspaceServiceTest, ListDirReturnsEntries) {
  const std::string conv = "conv-1";
  service_.SetWorkspaceRoot(conv, temp_dir_.GetPath());

  base::test::TestFuture<workspace::FileOpResult> future;
  service_.ListDir(conv, "", /*depth=*/0, future.GetCallback());
  workspace::FileOpResult result = future.Take();
  ASSERT_TRUE(result.has_value());
  EXPECT_NE(result->find("a.txt"), std::string::npos);
}

TEST_F(WorkspaceServiceTest, StrReplaceThenUndo) {
  const std::string conv = "conv-1";
  service_.SetWorkspaceRoot(conv, temp_dir_.GetPath());
  const base::FilePath file = temp_dir_.GetPath().AppendASCII("a.txt");

  base::test::TestFuture<workspace::FileOpResult> edit;
  service_.StrReplace(conv, "a.txt", "world", "there", edit.GetCallback());
  ASSERT_TRUE(edit.Take().has_value());
  std::string after;
  ASSERT_TRUE(base::ReadFileToString(file, &after));
  EXPECT_NE(after.find("there"), std::string::npos);

  base::test::TestFuture<workspace::FileOpResult> undo;
  service_.UndoEdit(conv, "a.txt", undo.GetCallback());
  ASSERT_TRUE(undo.Take().has_value());
  std::string restored;
  ASSERT_TRUE(base::ReadFileToString(file, &restored));
  EXPECT_EQ(restored, "hello\nworld\n");
}

TEST_F(WorkspaceServiceTest, WritesAllowedToggle) {
  const std::string conv = "conv-1";
  EXPECT_FALSE(service_.AreWritesAllowed(conv));
  service_.SetWritesAllowed(conv, true);
  EXPECT_TRUE(service_.AreWritesAllowed(conv));
  EXPECT_FALSE(service_.AreWritesAllowed("other-conv"));
  service_.SetWritesAllowed(conv, false);
  EXPECT_FALSE(service_.AreWritesAllowed(conv));
}

TEST_F(WorkspaceServiceTest, UndoWithoutEditReturnsError) {
  const std::string conv = "conv-1";
  service_.SetWorkspaceRoot(conv, temp_dir_.GetPath());
  base::test::TestFuture<workspace::FileOpResult> undo;
  service_.UndoEdit(conv, "a.txt", undo.GetCallback());
  EXPECT_FALSE(undo.Take().has_value());
}

}  // namespace ai_chat

