// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/workspace/workspace_tool.h"

#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/browser/workspace/workspace_service.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class WorkspaceToolTest : public testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    ASSERT_TRUE(base::WriteFile(temp_dir_.GetPath().AppendASCII("a.txt"),
                                "hello\nworld\n"));
    service_.SetWorkspaceRoot(conv_, temp_dir_.GetPath());
    tools_ = BuildWorkspaceTools(service_.GetWeakPtr(), conv_);
  }

 protected:
  WorkspaceTool* FindTool(std::string_view name) {
    for (auto& tool : tools_) {
      if (tool->Name() == name) {
        return tool.get();
      }
    }
    return nullptr;
  }

  std::string RunTool(WorkspaceTool* tool, const std::string& input) {
    base::test::TestFuture<Tool::ToolResult, Tool::ToolArtifacts> future;
    tool->UseTool(input, future.GetCallback());
    auto result = future.Take();
    const Tool::ToolResult& blocks = std::get<0>(result);
    if (blocks.empty() || !blocks[0]->is_text_content_block()) {
      return std::string();
    }
    return blocks[0]->get_text_content_block()->text;
  }

  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  const std::string conv_ = "conv-1";
  WorkspaceService service_;
  std::vector<std::unique_ptr<WorkspaceTool>> tools_;
};

TEST_F(WorkspaceToolTest, BuildsExpectedTools) {
  EXPECT_TRUE(FindTool("list_dir"));
  EXPECT_TRUE(FindTool("view_file"));
  EXPECT_TRUE(FindTool("grep"));
  EXPECT_TRUE(FindTool("glob"));
  EXPECT_TRUE(FindTool("create_file"));
  EXPECT_TRUE(FindTool("str_replace"));
  EXPECT_TRUE(FindTool("insert"));
  EXPECT_TRUE(FindTool("append_file"));
  EXPECT_TRUE(FindTool("undo_edit"));
}

TEST_F(WorkspaceToolTest, AppendFileWritesToDisk) {
  WorkspaceTool* tool = FindTool("append_file");
  ASSERT_TRUE(tool);
  std::string out =
      RunTool(tool, R"({"path":"a.txt","content":"\nmore"})");
  EXPECT_NE(out.find("Appended"), std::string::npos);
  std::string written;
  ASSERT_TRUE(
      base::ReadFileToString(temp_dir_.GetPath().AppendASCII("a.txt"),
                             &written));
  EXPECT_EQ(written, "hello\nworld\n\nmore");
}

TEST_F(WorkspaceToolTest, WriteToolsRequirePermissionUntilGranted) {
  WorkspaceTool* tool = FindTool("str_replace");
  ASSERT_TRUE(tool);
  mojom::ToolUseEventPtr event = mojom::ToolUseEvent::New();

  auto before = tool->RequiresUserInteractionBeforeHandling(*event);
  EXPECT_TRUE(std::holds_alternative<mojom::PermissionChallengePtr>(before));

  tool->UserPermissionGranted("tool-use-id");
  auto after = tool->RequiresUserInteractionBeforeHandling(*event);
  ASSERT_TRUE(std::holds_alternative<bool>(after));
  EXPECT_FALSE(std::get<bool>(after));
}

TEST_F(WorkspaceToolTest, WritesAllowedBypassesPermission) {
  WorkspaceTool* tool = FindTool("str_replace");
  ASSERT_TRUE(tool);
  mojom::ToolUseEventPtr event = mojom::ToolUseEvent::New();

  // Without a blanket allowance, a write tool challenges for permission.
  EXPECT_TRUE(std::holds_alternative<mojom::PermissionChallengePtr>(
      tool->RequiresUserInteractionBeforeHandling(*event)));

  // Granting the per-conversation allowance skips the challenge.
  service_.SetWritesAllowed(conv_, true);
  auto requirement = tool->RequiresUserInteractionBeforeHandling(*event);
  ASSERT_TRUE(std::holds_alternative<bool>(requirement));
  EXPECT_FALSE(std::get<bool>(requirement));
}

TEST_F(WorkspaceToolTest, CreateFileWritesToDisk) {
  WorkspaceTool* tool = FindTool("create_file");
  ASSERT_TRUE(tool);
  std::string out =
      RunTool(tool, R"({"path":"made.txt","file_text":"generated"})");
  EXPECT_NE(out.find("Created"), std::string::npos);
  std::string written;
  ASSERT_TRUE(base::ReadFileToString(temp_dir_.GetPath().AppendASCII("made.txt"),
                                     &written));
  EXPECT_EQ(written, "generated");
}

TEST_F(WorkspaceToolTest, ViewFileReturnsContent) {
  WorkspaceTool* tool = FindTool("view_file");
  ASSERT_TRUE(tool);
  EXPECT_NE(RunTool(tool, R"({"path":"a.txt"})").find("hello"),
            std::string::npos);
}

TEST_F(WorkspaceToolTest, ViewFileMissingRequiredArgErrors) {
  WorkspaceTool* tool = FindTool("view_file");
  ASSERT_TRUE(tool);
  EXPECT_NE(RunTool(tool, "{}").find("Error"), std::string::npos);
}

TEST_F(WorkspaceToolTest, GrepForwardsToService) {
  WorkspaceTool* tool = FindTool("grep");
  ASSERT_TRUE(tool);
  EXPECT_NE(RunTool(tool, R"({"pattern":"world"})").find("a.txt"),
            std::string::npos);
}

TEST_F(WorkspaceToolTest, ListDirForwardsToService) {
  WorkspaceTool* tool = FindTool("list_dir");
  ASSERT_TRUE(tool);
  EXPECT_NE(RunTool(tool, "{}").find("a.txt"), std::string::npos);
}

TEST_F(WorkspaceToolTest, ReadToolsSelfExecuteWithoutPermission) {
  WorkspaceTool* tool = FindTool("view_file");
  ASSERT_TRUE(tool);
  mojom::ToolUseEventPtr event = mojom::ToolUseEvent::New();
  auto requirement = tool->RequiresUserInteractionBeforeHandling(*event);
  ASSERT_TRUE(std::holds_alternative<bool>(requirement));
  EXPECT_FALSE(std::get<bool>(requirement));
}

}  // namespace ai_chat
