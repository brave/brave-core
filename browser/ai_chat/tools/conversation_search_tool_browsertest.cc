// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/conversation_search_tool.h"

#include <memory>
#include <string>
#include <vector>

#include "base/functional/bind.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/browser/ai_chat/ai_chat_embeddings_service_factory.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ai_chat/browser_tool_provider.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/embeddings/ai_chat_embeddings_service.h"
#include "brave/components/ai_chat/core/browser/embeddings/fake_embedder.h"
#include "brave/components/ai_chat/core/browser/test_utils.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/test/browser_test.h"

namespace ai_chat {

namespace {

std::vector<mojom::ConversationTurnPtr> MakeHistory(
    const std::string& query,
    const std::string& response) {
  std::vector<mojom::ConversationTurnPtr> history = CreateSampleChatHistory(1u);
  history[0]->text = query;
  history[1]->events->clear();
  history[1]->events->push_back(
      mojom::ConversationEntryEvent::NewCompletionEvent(
          mojom::CompletionEvent::New(response)));
  return history;
}

}  // namespace

// Runs the conversation search tool against a real index, embedded by a fake.
class ConversationSearchToolBrowserTest : public InProcessBrowserTest {
 public:
  ConversationSearchToolBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(features::kAIChatHistory);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    create_services_subscription_ =
        BrowserContextDependencyManager::GetInstance()
            ->RegisterCreateServicesCallbackForTesting(base::BindRepeating(
                &ConversationSearchToolBrowserTest::OnWillCreateServices,
                base::Unretained(this)));
  }

 protected:
  void OnWillCreateServices(content::BrowserContext* context) {
    AIChatEmbeddingsServiceFactory::GetInstance()->SetTestingFactory(
        context,
        base::BindOnce(&ConversationSearchToolBrowserTest::BuildService,
                       base::Unretained(this)));
  }

  std::unique_ptr<KeyedService> BuildService(content::BrowserContext* context) {
    service_built_ = true;
    return std::make_unique<AIChatEmbeddingsService>(
        AIChatServiceFactory::GetForBrowserContext(context),
        user_prefs::UserPrefs::Get(context),
        g_browser_process->os_crypt_async(), &embedder_,
        &embedder_metadata_provider_, context->GetPath());
  }

  AIChatService* ai_chat_service() {
    return AIChatServiceFactory::GetForBrowserContext(browser()->GetProfile());
  }

  // Persists a conversation of one query and its response.
  std::string AddConversation(const std::string& query,
                              const std::string& response) {
    ConversationHandler* conversation = ai_chat_service()->CreateConversation();
    conversation->SetChatHistoryForTesting(MakeHistory(query, response));
    return conversation->get_conversation_uuid();
  }

  FakeEmbedder embedder_;
  FakeEmbedderMetadataProvider embedder_metadata_provider_;
  bool service_built_ = false;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  base::CallbackListSubscription create_services_subscription_;
};

// The index starts with the profile, so stored conversations are indexed
// before Leo is first used.
IN_PROC_BROWSER_TEST_F(ConversationSearchToolBrowserTest, StartsWithProfile) {
  EXPECT_TRUE(service_built_);
}

IN_PROC_BROWSER_TEST_F(ConversationSearchToolBrowserTest,
                       SearchesOtherConversations) {
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return ai_chat_service()->IsStorageReady(); }));
  AddConversation("Tell me about my cat", "Cats sleep for most of the day.");
  const std::string current_uuid =
      AddConversation("What did we say about cats?", "Let me look.");
  AIChatEmbeddingsService* service =
      AIChatEmbeddingsServiceFactory::GetForBrowserContext(
          browser()->GetProfile());
  ASSERT_TRUE(service);
  ASSERT_TRUE(base::test::RunUntil([&] {
    return embedder_.HasEmbedded("Cats sleep for most of the day.") &&
           embedder_.HasEmbedded("What did we say about cats?") &&
           service->IsIndexingIdleForTesting();
  }));
  base::test::TestFuture<void> flushed;
  service->FlushForTesting(flushed.GetCallback());
  ASSERT_TRUE(flushed.Wait());

  ConversationHandler* current =
      ai_chat_service()->GetConversation(current_uuid);
  ASSERT_TRUE(current);
  auto* provider = static_cast<BrowserToolProvider*>(
      current->GetFirstToolProviderForTesting());
  ConversationSearchTool* tool =
      provider->GetConversationSearchToolForTesting();
  ASSERT_TRUE(tool);
  tool->UserPermissionGranted("tool_use");
  base::test::TestFuture<Tool::ToolResult, Tool::ToolArtifacts> result;
  tool->UseTool(R"({"query": "cat"})", result.GetCallback());
  const Tool::ToolResult& output = result.Get<Tool::ToolResult>();
  ASSERT_EQ(output.size(), 1u);
  const std::string& json = output[0]->get_text_content_block()->text;
  EXPECT_NE(json.find("Cats sleep for most of the day."), std::string::npos);
  // The conversation the search is made from is left out.
  EXPECT_EQ(json.find("What did we say about cats?"), std::string::npos);

  // What was found is also shown to the user, as cards.
  const Tool::ToolArtifacts& artifacts = result.Get<Tool::ToolArtifacts>();
  ASSERT_EQ(artifacts.size(), 1u);
  EXPECT_EQ(artifacts[0]->type, mojom::kConversationSearchResultsArtifactType);
  EXPECT_NE(artifacts[0]->content_json.find("Tell me about my cat"),
            std::string::npos);
  EXPECT_EQ(artifacts[0]->content_json.find("What did we say about cats?"),
            std::string::npos);
}

// Without the testing factory, the Semantic history search setting, which is
// off by default, decides.
class ConversationSearchToolOffBrowserTest : public InProcessBrowserTest {};

IN_PROC_BROWSER_TEST_F(ConversationSearchToolOffBrowserTest,
                       NotOfferedWhileSemanticSearchIsOff) {
  EXPECT_FALSE(AIChatEmbeddingsServiceFactory::GetForBrowserContext(
      browser()->GetProfile()));
  ConversationHandler* conversation =
      AIChatServiceFactory::GetForBrowserContext(browser()->GetProfile())
          ->CreateConversation();
  auto* provider = static_cast<BrowserToolProvider*>(
      conversation->GetFirstToolProviderForTesting());
  ASSERT_TRUE(provider);
  EXPECT_FALSE(provider->GetConversationSearchToolForTesting());
}

}  // namespace ai_chat
