// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/workspace/workspace_file_ops.h"

#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat::workspace {

namespace {

bool Contains(const std::string& haystack, const std::string& needle) {
  return haystack.find(needle) != std::string::npos;
}

}  // namespace

class WorkspaceFileOpsTest : public testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    // Canonicalize the root exactly as WorkspaceService does, so symlink
    // containment checks behave the same as in production.
    root_ = base::MakeAbsoluteFilePath(temp_dir_.GetPath());
    ASSERT_FALSE(root_.empty());

    ASSERT_TRUE(base::WriteFile(root_.AppendASCII("a.txt"), "hello\nworld\n"));
    ASSERT_TRUE(base::WriteFile(root_.AppendASCII("b.log"), "error here\n"));
    ASSERT_TRUE(base::CreateDirectory(root_.AppendASCII("sub")));
    ASSERT_TRUE(base::WriteFile(root_.AppendASCII("sub").AppendASCII("c.txt"),
                                "nested\nmatch me\n"));
  }

 protected:
  base::ScopedTempDir temp_dir_;
  base::FilePath root_;
};

TEST_F(WorkspaceFileOpsTest, ResolveWithinRoot_Empty) {
  EXPECT_EQ(ResolveWithinRoot(root_, "", /*require_exists=*/true), root_);
}

TEST_F(WorkspaceFileOpsTest, ResolveWithinRoot_SimpleChild) {
  auto resolved = ResolveWithinRoot(root_, "a.txt", /*require_exists=*/true);
  ASSERT_TRUE(resolved);
  EXPECT_EQ(*resolved, root_.AppendASCII("a.txt"));
}

TEST_F(WorkspaceFileOpsTest, ResolveWithinRoot_RejectsAbsolute) {
  EXPECT_FALSE(
      ResolveWithinRoot(root_, "/etc/passwd", /*require_exists=*/false));
}

TEST_F(WorkspaceFileOpsTest, ResolveWithinRoot_RejectsParentTraversal) {
  EXPECT_FALSE(ResolveWithinRoot(root_, "../secret", /*require_exists=*/false));
  EXPECT_FALSE(
      ResolveWithinRoot(root_, "sub/../../secret", /*require_exists=*/false));
}

TEST_F(WorkspaceFileOpsTest, ResolveWithinRoot_MissingFileRequireExists) {
  EXPECT_FALSE(
      ResolveWithinRoot(root_, "nope.txt", /*require_exists=*/true));
  // Same path is allowed (lexically) when existence isn't required (writes).
  EXPECT_TRUE(ResolveWithinRoot(root_, "nope.txt", /*require_exists=*/false));
}

TEST_F(WorkspaceFileOpsTest, ResolveWithinRoot_AcceptsAbsoluteWithinRoot) {
  // Agents often pass absolute paths; accept them when inside the root.
  auto file = ResolveWithinRoot(root_, root_.AppendASCII("a.txt").AsUTF8Unsafe(),
                                /*require_exists=*/true);
  ASSERT_TRUE(file);
  EXPECT_EQ(*file, root_.AppendASCII("a.txt"));

  // The root's own absolute path resolves to the root.
  EXPECT_EQ(ResolveWithinRoot(root_, root_.AsUTF8Unsafe(),
                              /*require_exists=*/true),
            root_);
}

TEST_F(WorkspaceFileOpsTest, ResolveWithinRoot_RejectsAbsoluteOutsideRoot) {
  base::ScopedTempDir outside;
  ASSERT_TRUE(outside.CreateUniqueTempDir());
  ASSERT_TRUE(base::WriteFile(outside.GetPath().AppendASCII("x.txt"), "x"));
  EXPECT_FALSE(ResolveWithinRoot(
      root_, outside.GetPath().AppendASCII("x.txt").AsUTF8Unsafe(),
      /*require_exists=*/true));
}

TEST_F(WorkspaceFileOpsTest, Glob_AbsolutePathEqualToRootSearchesWholeTree) {
  FileOpResult result = Glob(root_, root_.AsUTF8Unsafe(), "*.txt");
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(Contains(*result, "a.txt"));
}

#if BUILDFLAG(IS_POSIX)
TEST_F(WorkspaceFileOpsTest, ResolveWithinRoot_RejectsSymlinkEscape) {
  base::ScopedTempDir outside;
  ASSERT_TRUE(outside.CreateUniqueTempDir());
  ASSERT_TRUE(base::WriteFile(outside.GetPath().AppendASCII("secret.txt"),
                              "top secret"));
  // A symlink inside the root that points outside it.
  ASSERT_TRUE(base::CreateSymbolicLink(outside.GetPath(),
                                       root_.AppendASCII("link")));

  // The symlink itself resolves outside the root -> rejected.
  EXPECT_FALSE(
      ResolveWithinRoot(root_, "link/secret.txt", /*require_exists=*/true));
}
#endif

TEST_F(WorkspaceFileOpsTest, ListDir_ListsEntries) {
  FileOpResult result = ListDir(root_, "", kDefaultListDepth);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(Contains(*result, "a.txt"));
  EXPECT_TRUE(Contains(*result, "b.log"));
  EXPECT_TRUE(Contains(*result, "sub/"));
  // depth 2 includes the nested file.
  EXPECT_TRUE(Contains(*result, "sub/c.txt"));
}

TEST_F(WorkspaceFileOpsTest, ListDir_DepthOneExcludesNested) {
  FileOpResult result = ListDir(root_, "", /*depth=*/1);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(Contains(*result, "sub/"));
  EXPECT_FALSE(Contains(*result, "c.txt"));
}

TEST_F(WorkspaceFileOpsTest, ReadFile_NumbersLines) {
  FileOpResult result =
      ReadFile(root_, "a.txt", std::nullopt, std::nullopt);
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(Contains(*result, "1\thello"));
  EXPECT_TRUE(Contains(*result, "2\tworld"));
}

TEST_F(WorkspaceFileOpsTest, ReadFile_Range) {
  FileOpResult result = ReadFile(root_, "a.txt", 2, 2);
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(Contains(*result, "hello"));
  EXPECT_TRUE(Contains(*result, "2\tworld"));
}

TEST_F(WorkspaceFileOpsTest, ReadFile_RejectsEscape) {
  FileOpResult result =
      ReadFile(root_, "../outside.txt", std::nullopt, std::nullopt);
  EXPECT_FALSE(result.has_value());
}

TEST_F(WorkspaceFileOpsTest, Grep_FindsMatchesWithIncludeFilter) {
  FileOpResult result = Grep(root_, "", "match", "*.txt");
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(Contains(*result, "sub/c.txt"));
  EXPECT_TRUE(Contains(*result, "match me"));
}

TEST_F(WorkspaceFileOpsTest, Grep_IncludeFilterExcludesOtherExtensions) {
  FileOpResult result = Grep(root_, "", "error", "*.txt");
  ASSERT_TRUE(result.has_value());
  // b.log contains "error" but is excluded by the *.txt include filter.
  EXPECT_FALSE(Contains(*result, "b.log"));
}

TEST_F(WorkspaceFileOpsTest, Grep_InvalidRegex) {
  FileOpResult result = Grep(root_, "", "(unclosed", "");
  EXPECT_FALSE(result.has_value());
}

TEST_F(WorkspaceFileOpsTest, Glob_MatchesByPattern) {
  FileOpResult result = Glob(root_, "", "*.txt");
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(Contains(*result, "a.txt"));
  EXPECT_TRUE(Contains(*result, "sub/c.txt"));
  EXPECT_FALSE(Contains(*result, "b.log"));
}

TEST_F(WorkspaceFileOpsTest, Glob_MatchesBareFileNameAtAnyDepth) {
  // A bare file name matches a nested file via basename matching.
  FileOpResult result = Glob(root_, "", "c.txt");
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(Contains(*result, "sub/c.txt"));
}

TEST_F(WorkspaceFileOpsTest, CreateFile_WritesNewFile) {
  WriteResult result = CreateFile(root_, "new.txt", "fresh content");
  ASSERT_TRUE(result.has_value());
  EXPECT_FALSE(result->previous_content.has_value());
  std::string written;
  ASSERT_TRUE(base::ReadFileToString(root_.AppendASCII("new.txt"), &written));
  EXPECT_EQ(written, "fresh content");
}

TEST_F(WorkspaceFileOpsTest, CreateFile_FailsIfExists) {
  EXPECT_FALSE(CreateFile(root_, "a.txt", "x").has_value());
}

TEST_F(WorkspaceFileOpsTest, CreateFile_RejectsEscape) {
  EXPECT_FALSE(CreateFile(root_, "../evil.txt", "x").has_value());
}

TEST_F(WorkspaceFileOpsTest, StrReplace_UniqueMatch) {
  WriteResult result = StrReplace(root_, "a.txt", "world", "there");
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->previous_content.has_value());
  EXPECT_EQ(*result->previous_content, "hello\nworld\n");
  std::string written;
  ASSERT_TRUE(base::ReadFileToString(root_.AppendASCII("a.txt"), &written));
  EXPECT_EQ(written, "hello\nthere\n");
}

TEST_F(WorkspaceFileOpsTest, StrReplace_NotFound) {
  EXPECT_FALSE(StrReplace(root_, "a.txt", "absent", "x").has_value());
}

TEST_F(WorkspaceFileOpsTest, StrReplace_NotUnique) {
  ASSERT_TRUE(base::WriteFile(root_.AppendASCII("dup.txt"), "x\nx\n"));
  EXPECT_FALSE(StrReplace(root_, "dup.txt", "x", "y").has_value());
}

TEST_F(WorkspaceFileOpsTest, Insert_AddsLine) {
  WriteResult result = Insert(root_, "a.txt", 1, "middle");
  ASSERT_TRUE(result.has_value());
  std::string written;
  ASSERT_TRUE(base::ReadFileToString(root_.AppendASCII("a.txt"), &written));
  EXPECT_EQ(written, "hello\nmiddle\nworld\n");
}

TEST_F(WorkspaceFileOpsTest, RepoMap_ExtractsDefinitions) {
  ASSERT_TRUE(base::WriteFile(
      root_.AppendASCII("code.ts"),
      "import x from 'y'\n"
      "export class Widget {\n"
      "  build() {}\n"
      "}\n"
      "function helper() {}\n"));
  FileOpResult result = RepoMap(root_, "");
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(Contains(*result, "code.ts:"));
  EXPECT_TRUE(Contains(*result, "export class Widget"));
  EXPECT_TRUE(Contains(*result, "function helper()"));
  // Non-definition lines are omitted.
  EXPECT_FALSE(Contains(*result, "import x"));
  // Non-source files (a.txt) are not mapped.
  EXPECT_FALSE(Contains(*result, "a.txt:"));
}

TEST_F(WorkspaceFileOpsTest, AppendFile_AppendsToExisting) {
  WriteResult result = AppendFile(root_, "a.txt", "appended\n");
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->previous_content.has_value());
  EXPECT_EQ(*result->previous_content, "hello\nworld\n");
  std::string written;
  ASSERT_TRUE(base::ReadFileToString(root_.AppendASCII("a.txt"), &written));
  EXPECT_EQ(written, "hello\nworld\nappended\n");
}

TEST_F(WorkspaceFileOpsTest, AppendFile_CreatesWhenMissing) {
  WriteResult result = AppendFile(root_, "brandnew.txt", "first chunk");
  ASSERT_TRUE(result.has_value());
  // No previous content => undo would delete it.
  EXPECT_FALSE(result->previous_content.has_value());
  std::string written;
  ASSERT_TRUE(
      base::ReadFileToString(root_.AppendASCII("brandnew.txt"), &written));
  EXPECT_EQ(written, "first chunk");
}

TEST_F(WorkspaceFileOpsTest, AppendFile_RejectsEscape) {
  EXPECT_FALSE(AppendFile(root_, "../evil.txt", "x").has_value());
}

TEST_F(WorkspaceFileOpsTest, RestoreFile_RestoresAndDeletes) {
  // Restore previous content.
  ASSERT_TRUE(RestoreFile(root_.AppendASCII("a.txt"), "restored").has_value());
  std::string written;
  ASSERT_TRUE(base::ReadFileToString(root_.AppendASCII("a.txt"), &written));
  EXPECT_EQ(written, "restored");

  // Undo of a created file (nullopt) deletes it.
  base::FilePath created = root_.AppendASCII("created.txt");
  ASSERT_TRUE(base::WriteFile(created, "temp"));
  ASSERT_TRUE(RestoreFile(created, std::nullopt).has_value());
  EXPECT_FALSE(base::PathExists(created));
}

}  // namespace ai_chat::workspace
