/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/bind.h"
#include "base/values.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_devtools_protocol_client.h"

namespace ai_chat {

class AIChatCDPBrowserTest : public InProcessBrowserTest,
                             public content::TestDevToolsProtocolClient {
 public:
  AIChatCDPBrowserTest() {
    feature_list_.InitWithFeatures({features::kAIChatCDP}, {});
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    // Accept Leo disclaimer so messages aren't queued.
    auto* profile = browser()->profile();
    SetUserOptedIn(profile->GetPrefs(), true);
    AttachToBrowserTarget();
  }

  void TearDownOnMainThread() override {
    DetachProtocolClient();
    InProcessBrowserTest::TearDownOnMainThread();
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(AIChatCDPBrowserTest, EnableDisable) {
  const auto* result = SendCommandSync("BraveAIChat.enable");
  ASSERT_TRUE(result);
  EXPECT_FALSE(error());

  result = SendCommandSync("BraveAIChat.disable");
  ASSERT_TRUE(result);
  EXPECT_FALSE(error());
}

IN_PROC_BROWSER_TEST_F(AIChatCDPBrowserTest, EnableTwiceIsIdempotent) {
  SendCommandSync("BraveAIChat.enable");
  EXPECT_FALSE(error());
  SendCommandSync("BraveAIChat.enable");
  EXPECT_FALSE(error());
  SendCommandSync("BraveAIChat.disable");
}

IN_PROC_BROWSER_TEST_F(AIChatCDPBrowserTest, CreateAndDestroyConversation) {
  SendCommandSync("BraveAIChat.enable");

  const auto* result = SendCommandSync("BraveAIChat.createConversation");
  ASSERT_TRUE(result);
  EXPECT_FALSE(error());

  const std::string* conversation_id = result->FindString("conversationId");
  ASSERT_TRUE(conversation_id);
  EXPECT_FALSE(conversation_id->empty());

  base::DictValue params;
  params.Set("conversationId", *conversation_id);
  SendCommandSync("BraveAIChat.destroyConversation", std::move(params));
  EXPECT_FALSE(error());

  SendCommandSync("BraveAIChat.disable");
}

IN_PROC_BROWSER_TEST_F(AIChatCDPBrowserTest, GetHistoryEmpty) {
  SendCommandSync("BraveAIChat.enable");

  const auto* create_result = SendCommandSync("BraveAIChat.createConversation");
  const std::string* conversation_id =
      create_result->FindString("conversationId");

  base::DictValue params;
  params.Set("conversationId", *conversation_id);
  const auto* history_result =
      SendCommandSync("BraveAIChat.getHistory", std::move(params));
  ASSERT_TRUE(history_result);
  EXPECT_FALSE(error());

  const auto* entries = history_result->FindList("entries");
  ASSERT_TRUE(entries);
  EXPECT_TRUE(entries->empty());

  base::DictValue destroy_params;
  destroy_params.Set("conversationId", *conversation_id);
  SendCommandSync("BraveAIChat.destroyConversation", std::move(destroy_params));
  SendCommandSync("BraveAIChat.disable");
}

IN_PROC_BROWSER_TEST_F(AIChatCDPBrowserTest, CommandsFailWhenNotEnabled) {
  // createConversation should fail without enable
  SendCommandSync("BraveAIChat.createConversation");
  EXPECT_TRUE(error());
}

IN_PROC_BROWSER_TEST_F(AIChatCDPBrowserTest, DestroyUnknownConversationFails) {
  SendCommandSync("BraveAIChat.enable");

  base::DictValue params;
  params.Set("conversationId", "nonexistent-uuid");
  SendCommandSync("BraveAIChat.destroyConversation", std::move(params));
  EXPECT_TRUE(error());

  SendCommandSync("BraveAIChat.disable");
}

IN_PROC_BROWSER_TEST_F(AIChatCDPBrowserTest, SubmitMessageToUnknownFails) {
  SendCommandSync("BraveAIChat.enable");

  base::DictValue params;
  params.Set("conversationId", "nonexistent-uuid");
  params.Set("message", "hello");
  SendCommandSync("BraveAIChat.submitMessage", std::move(params));
  EXPECT_TRUE(error());

  SendCommandSync("BraveAIChat.disable");
}

}  // namespace ai_chat
