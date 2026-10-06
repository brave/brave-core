// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/attach_workspace_tool.h"

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ai_chat/browser_tool_provider.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/associated_content_manager.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/types.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class AttachWorkspaceToolTest : public ChromeRenderViewHostTestHarness {
 public:
  AttachWorkspaceToolTest() {
    scoped_feature_list_.InitAndEnableFeature(features::kAIChatWorkspaceTools);
  }

  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    auto* service =
        AIChatServiceFactory::GetForBrowserContext(GetBrowserContext());
    ASSERT_TRUE(service);
    conversation_ = service->CreateConversation();
    ASSERT_TRUE(conversation_);
    tool_ = std::make_unique<AttachWorkspaceTool>(GetBrowserContext(),
                                                  conversation_);
  }

  void TearDown() override {
    // The workspace page's WebContents is owned by the conversation, which
    // outlives the test harness. Drop it here so the harness doesn't report a
    // leaked RenderWidgetHost.
    for (const auto& content : associated_content()) {
      manager()->RemoveContent(content->uuid);
    }
    tool_.reset();
    conversation_ = nullptr;
    ChromeRenderViewHostTestHarness::TearDown();
  }

  // Runs the tool and returns its text output.
  std::string UseTool() {
    base::test::TestFuture<std::vector<mojom::ContentBlockPtr>,
                           std::vector<mojom::ToolArtifactPtr>>
        future;
    tool_->UseTool("{}", future.GetCallback());
    auto& output = future.Get<std::vector<mojom::ContentBlockPtr>>();
    EXPECT_EQ(1u, output.size());
    return output.empty() ? "" : output[0]->get_text_content_block()->text;
  }

  AssociatedContentManager* manager() {
    return conversation_->associated_content_manager();
  }

  std::vector<mojom::AssociatedContentPtr> associated_content() {
    return manager()->GetAssociatedContent();
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
  raw_ptr<ConversationHandler> conversation_ = nullptr;
  std::unique_ptr<AttachWorkspaceTool> tool_;
};

TEST_F(AttachWorkspaceToolTest, AttachesAnEmptyWorkspace) {
  EXPECT_EQ(mojom::kAttachWorkspaceToolName, tool_->Name());
  EXPECT_FALSE(tool_->Description().empty());

  EXPECT_NE(std::string::npos, UseTool().find("Attached an empty workspace"));

  auto content = associated_content();
  ASSERT_EQ(1u, content.size());
  EXPECT_EQ(mojom::ContentType::Workspace, content[0]->content_type);
}

TEST_F(AttachWorkspaceToolTest, DoesNotAttachASecondWorkspace) {
  UseTool();
  ASSERT_EQ(1u, associated_content().size());

  EXPECT_EQ("This conversation already has a workspace.", UseTool());
  EXPECT_EQ(1u, associated_content().size());
}

TEST_F(AttachWorkspaceToolTest, OnlySupportsConversationsWithoutAWorkspace) {
  EXPECT_TRUE(tool_->SupportsConversation(
      /*is_temporary=*/false, /*has_untrusted_content=*/false, {}));
  EXPECT_FALSE(tool_->SupportsConversation(
      /*is_temporary=*/false, /*has_untrusted_content=*/false,
      {mojom::ConversationCapability::WORKSPACES}));
}

TEST_F(AttachWorkspaceToolTest, ProvidedByBrowserToolProvider) {
  BrowserToolProvider provider(profile(), conversation_);
  bool found = false;
  for (const auto& tool : provider.GetTools()) {
    found |= tool && tool->Name() == mojom::kAttachWorkspaceToolName;
  }
  EXPECT_TRUE(found);
}

class AttachWorkspaceToolDisabledTest : public ChromeRenderViewHostTestHarness {
};

// kAIChatWorkspaceTools is disabled by default, and the tool is experimental.
TEST_F(AttachWorkspaceToolDisabledTest, NotProvidedWhenFeatureDisabled) {
  auto* service =
      AIChatServiceFactory::GetForBrowserContext(GetBrowserContext());
  ASSERT_TRUE(service);
  BrowserToolProvider provider(profile(), service->CreateConversation());
  for (const auto& tool : provider.GetTools()) {
    ASSERT_TRUE(tool);
    EXPECT_NE(mojom::kAttachWorkspaceToolName, tool->Name());
  }
}

}  // namespace ai_chat
