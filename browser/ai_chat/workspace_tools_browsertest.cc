// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_helpers.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/threading/thread_restrictions.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/components/ai_chat/content/browser/workspace_associated_content.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/associated_content_manager.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

// The number of file tools registered by the workspace page (see tools.ts):
// str_replace_based_edit_tool, grep, glob.
constexpr size_t kExpectedToolCount = 3;

std::string TextOf(const Tool::ToolResult& output) {
  std::string text;
  for (const auto& block : output) {
    if (block->is_text_content_block()) {
      text += block->get_text_content_block()->text;
    }
  }
  return text;
}

}  // namespace

// Exercises the full Leo workspace pipeline without a live model: a hidden
// chrome-untrusted://workspace page receives a FileSystemDirectoryHandle for a
// picked folder (via launchQueue), registers its file tools via WebMCP, and Leo
// harvests them. Also runs list_dir end-to-end to confirm the handle is usable.
class WorkspaceToolsBrowserTest : public InProcessBrowserTest {
 public:
  WorkspaceToolsBrowserTest() {
    scoped_feature_list_.InitWithFeatures(
        /*enabled_features=*/{features::kAIChatWorkspaceTools},
        /*disabled_features=*/{});
  }
  ~WorkspaceToolsBrowserTest() override = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    // navigator.modelContext is gated by the (experimental) WebMCP runtime
    // feature; enable it in every renderer for the test.
    command_line->AppendSwitchASCII("enable-blink-features", "WebMCP");
  }

 protected:
  base::FilePath CreateWorkspaceFolder() {
    base::ScopedAllowBlockingForTesting allow_blocking;
    EXPECT_TRUE(temp_dir_.CreateUniqueTempDir());
    const base::FilePath root = temp_dir_.GetPath();
    EXPECT_TRUE(base::WriteFile(root.AppendASCII("hello.txt"), "hello world"));
    EXPECT_TRUE(base::CreateDirectory(root.AppendASCII("sub")));
    EXPECT_TRUE(
        base::WriteFile(root.AppendASCII("sub").AppendASCII("world.txt"), "w"));
    return root;
  }

  std::vector<base::WeakPtr<Tool>> RefreshAndGetTools(
      AssociatedContentManager* manager) {
    base::test::TestFuture<void> done;
    manager->UpdateToolsForNewGenerationLoop(done.GetCallback());
    EXPECT_TRUE(done.Wait());
    return manager->GetTools();
  }

  base::ScopedTempDir temp_dir_;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(WorkspaceToolsBrowserTest,
                       RegistersToolsAndReadsFolder) {
  const base::FilePath folder = CreateWorkspaceFolder();

  auto content = std::make_unique<WorkspaceAssociatedContent>(
      folder, browser()->profile(), base::DoNothing());
  auto* content_ptr = content.get();
  content::WebContents* web_contents = content_ptr->GetWebContentsForTesting();
  ASSERT_TRUE(web_contents);

  // The delegate started navigating to chrome-untrusted://workspace/<guid> in
  // its constructor; wait for that to commit before evaluating script, so the
  // poll below runs in the workspace document rather than the initial one.
  ASSERT_TRUE(content::WaitForLoadStop(web_contents));

  // Wait for the page to load, receive its handle via launchQueue, and register
  // its tools. Poll from JS (a single EvalJs with an internal loop) rather than
  // wrapping EvalJs in a C++ RunUntil.
  ASSERT_EQ(static_cast<int>(kExpectedToolCount),
            content::EvalJs(web_contents, R"(
      (async () => {
        for (let i = 0; i < 200; i++) {
          if (navigator.modelContext) {
            const tools = await navigator.modelContext.getTools();
            if (tools.length >= 3) {
              return tools.length;
            }
          }
          await new Promise((r) => setTimeout(r, 50));
        }
        return navigator.modelContext
            ? (await navigator.modelContext.getTools()).length
            : -1;
      })()
  )"));

  // Attach the workspace page to a conversation and confirm Leo harvests the
  // tools through the normal content-tools pipeline.
  auto* service =
      AIChatServiceFactory::GetForBrowserContext(browser()->profile());
  ASSERT_TRUE(service);
  auto* conversation = service->CreateConversation();
  ASSERT_TRUE(conversation);
  auto* manager = conversation->associated_content_manager();
  manager->AddOwnedContent(std::move(content));
  ASSERT_TRUE(
      base::test::RunUntil([&] { return content_ptr->tools_attached(); }));

  auto tools = RefreshAndGetTools(manager);
  EXPECT_EQ(kExpectedToolCount, tools.size());

  std::set<std::string> names;
  for (const auto& tool : tools) {
    ASSERT_TRUE(tool);
    names.insert(std::string(tool->Name()));
  }
  auto has_suffix = [&](std::string_view suffix) {
    for (const auto& name : names) {
      if (std::string_view(name).ends_with(suffix)) {
        return true;
      }
    }
    return false;
  };
  EXPECT_TRUE(has_suffix("str_replace_based_edit_tool"));
  EXPECT_TRUE(has_suffix("grep"));
  EXPECT_TRUE(has_suffix("glob"));

  // Execute the editor tool's `view` command on the root directory end-to-end:
  // this proves the delivered handle is usable and the FS Access read reflects
  // the real folder contents.
  Tool* editor = nullptr;
  for (const auto& tool : tools) {
    if (tool && std::string_view(tool->Name())
                    .ends_with("str_replace_based_edit_tool")) {
      editor = tool.get();
      break;
    }
  }
  ASSERT_TRUE(editor);

  base::test::TestFuture<Tool::ToolResult, Tool::ToolArtifacts> result;
  editor->UseTool(R"({"command": "view", "path": ""})", result.GetCallback());
  const std::string text = TextOf(std::get<0>(result.Take()));
  EXPECT_NE(text.find("hello.txt"), std::string::npos) << text;
  EXPECT_NE(text.find("sub/"), std::string::npos) << text;
}

}  // namespace ai_chat
