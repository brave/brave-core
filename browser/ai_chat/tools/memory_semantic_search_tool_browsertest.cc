// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/memory_semantic_search_tool.h"

#include <memory>
#include <string>

#include "base/functional/bind.h"
#include "base/test/run_until.h"
#include "base/test/test_future.h"
#include "brave/browser/ai_chat/ai_chat_embeddings_service_factory.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ai_chat/browser_tool_provider.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/embeddings/ai_chat_embeddings_service.h"
#include "brave/components/ai_chat/core/browser/embeddings/fake_embedder.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/ai_chat/core/common/prefs.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/test/browser_test.h"

namespace ai_chat {

// Runs the memory search tool against a real index, embedded by a fake.
class MemorySemanticSearchToolBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    create_services_subscription_ =
        BrowserContextDependencyManager::GetInstance()
            ->RegisterCreateServicesCallbackForTesting(base::BindRepeating(
                &MemorySemanticSearchToolBrowserTest::OnWillCreateServices,
                base::Unretained(this)));
  }

 protected:
  void OnWillCreateServices(content::BrowserContext* context) {
    AIChatEmbeddingsServiceFactory::GetInstance()->SetTestingFactory(
        context,
        base::BindOnce(&MemorySemanticSearchToolBrowserTest::BuildService,
                       base::Unretained(this)));
  }

  std::unique_ptr<KeyedService> BuildService(content::BrowserContext* context) {
    return std::make_unique<AIChatEmbeddingsService>(
        AIChatServiceFactory::GetForBrowserContext(context),
        user_prefs::UserPrefs::Get(context),
        g_browser_process->os_crypt_async(), &embedder_,
        &embedder_metadata_provider_, context->GetPath());
  }

  FakeEmbedder embedder_;
  FakeEmbedderMetadataProvider embedder_metadata_provider_;

 private:
  base::CallbackListSubscription create_services_subscription_;
};

IN_PROC_BROWSER_TEST_F(MemorySemanticSearchToolBrowserTest, SearchesMemories) {
  PrefService* prefs = browser()->GetProfile()->GetPrefs();
  prefs->SetBoolean(prefs::kBraveAIChatUserMemoryEnabled, true);
  ConversationHandler* conversation =
      AIChatServiceFactory::GetForBrowserContext(browser()->GetProfile())
          ->CreateConversation();
  auto* provider = static_cast<BrowserToolProvider*>(
      conversation->GetFirstToolProviderForTesting());
  MemorySemanticSearchTool* tool =
      provider->GetMemorySemanticSearchToolForTesting();
  ASSERT_TRUE(tool);

  prefs::AddMemoryToPrefs("Has a cat named Tom", *prefs);
  prefs::AddMemoryToPrefs("Walks the dog daily", *prefs);
  AIChatEmbeddingsService* service =
      AIChatEmbeddingsServiceFactory::GetForBrowserContext(
          browser()->GetProfile());
  ASSERT_TRUE(base::test::RunUntil([&] {
    return embedder_.HasEmbedded("Walks the dog daily") &&
           service->IsIndexingIdleForTesting();
  }));
  base::test::TestFuture<void> flushed;
  service->FlushForTesting(flushed.GetCallback());
  ASSERT_TRUE(flushed.Wait());

  base::test::TestFuture<Tool::ToolResult, Tool::ToolArtifacts> result;
  tool->UseTool(R"({"query": "dog"})", result.GetCallback());
  const Tool::ToolResult& output = result.Get<Tool::ToolResult>();
  ASSERT_EQ(output.size(), 1u);
  EXPECT_EQ(output[0]->get_text_content_block()->text,
            R"({"memories":["Walks the dog daily"],"query":"dog"})");
}

}  // namespace ai_chat
