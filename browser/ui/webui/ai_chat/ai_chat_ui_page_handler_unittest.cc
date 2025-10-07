// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/ai_chat_ui_page_handler.h"

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_tab_helper.h"
#include "brave/components/ai_chat/content/browser/associated_url_content.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

class AIChatUIPageHandlerTest : public ChromeRenderViewHostTestHarness {
 public:
  AIChatUIPageHandlerTest() = default;
  ~AIChatUIPageHandlerTest() override = default;

  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();

    // Create the AIChatService
    service_ = AIChatServiceFactory::GetForBrowserContext(GetBrowserContext());
    ASSERT_TRUE(service_);

    // Create page handler
    mojo::PendingReceiver<mojom::AIChatUIHandler> receiver;
    page_handler_ = std::make_unique<AIChatUIPageHandler>(
        web_contents(), nullptr,
        Profile::FromBrowserContext(GetBrowserContext()), std::move(receiver));
  }

  void TearDown() override {
    service_ = nullptr;
    page_handler_.reset();
    ChromeRenderViewHostTestHarness::TearDown();
  }

  AIChatService* service() { return service_; }
  AIChatUIPageHandler* page_handler() { return page_handler_.get(); }

 private:
  raw_ptr<AIChatService> service_ = nullptr;
  std::unique_ptr<AIChatUIPageHandler> page_handler_;
};

TEST_F(AIChatUIPageHandlerTest, AssociateUrlContent_ValidHttpsUrl) {
  // Create a conversation
  auto* conversation = service()->CreateConversation();
  ASSERT_TRUE(conversation);
  std::string conversation_uuid = conversation->get_conversation_uuid();

  // Associate a URL with the conversation
  GURL test_url("https://example.com/test");
  std::string title = "Test Page";
  page_handler()->AssociateUrlContent(test_url, title, conversation_uuid);

  auto associated_content =
      conversation->associated_content_manager()->GetAssociatedContent();
  ASSERT_EQ(associated_content.size(), 1u);
  EXPECT_EQ(associated_content[0]->url, test_url);
  EXPECT_EQ(associated_content[0]->title, title);

  // The delegate should have been created.
  auto delegates = conversation->associated_content_manager()
                       ->GetContentDelegatesForTesting();
  ASSERT_EQ(delegates.size(), 1u);
  EXPECT_EQ(delegates[0]->url(), test_url);
  EXPECT_EQ(delegates[0]->title(), base::UTF8ToUTF16(title));

  // It should be an AssociatedURLContent
  auto* associated_link_content =
      static_cast<AssociatedURLContent*>(delegates[0]);

  // Should have attached shields observer and ephemeral storage tab helper
  EXPECT_TRUE(brave_shields::BraveShieldsWebContentsObserver::FromWebContents(
      associated_link_content->GetWebContentsForTesting()));
  EXPECT_TRUE(ephemeral_storage::EphemeralStorageTabHelper::FromWebContents(
      associated_link_content->GetWebContentsForTesting()));

  // Should be possible to disassociate the content
  page_handler()->DisassociateContent(std::move(associated_content[0]),
                                      conversation_uuid);

  associated_content =
      conversation->associated_content_manager()->GetAssociatedContent();
  EXPECT_TRUE(associated_content.empty());
}

TEST_F(AIChatUIPageHandlerTest, AssociateUrlContent_InvalidScheme) {
  // Create a conversation
  auto* conversation = service()->CreateConversation();
  ASSERT_TRUE(conversation);
  std::string conversation_uuid = conversation->get_conversation_uuid();

  // Try to associate a chrome:// URL (disallowed scheme)
  GURL chrome_url("chrome://settings");
  std::string title = "Settings Page";
  page_handler()->AssociateUrlContent(chrome_url, title, conversation_uuid);

  // Verify the content was NOT associated due to invalid scheme
  auto associated_content =
      conversation->associated_content_manager()->GetAssociatedContent();
  EXPECT_TRUE(associated_content.empty());
}

TEST_F(AIChatUIPageHandlerTest, AssociateUrlContent_InvalidConversation) {
  // Try to associate with a non-existent conversation
  GURL test_url("https://example.com/test");
  std::string title = "Test Page";
  page_handler()->AssociateUrlContent(test_url, title, "non-existent-uuid");

  // Should not crash
}

}  // namespace ai_chat
