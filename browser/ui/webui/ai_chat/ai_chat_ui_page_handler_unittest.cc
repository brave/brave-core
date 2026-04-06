// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/ai_chat_ui_page_handler.h"

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/test_future.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_tab_helper.h"
#include "brave/components/ai_chat/content/browser/associated_url_content.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "build/build_config.h"
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

TEST_F(AIChatUIPageHandlerTest, FinishUpload_StripsPathToBasename) {
  std::vector<mojom::UploadedFilePtr> files;
  files.push_back(mojom::UploadedFile::New(
      "/home/user/documents/report.pdf", 100, std::vector<uint8_t>(100),
      mojom::UploadedFileType::kPdf, std::nullopt));
  files.push_back(
      mojom::UploadedFile::New("/tmp/image.png", 50, std::vector<uint8_t>(50),
                               mojom::UploadedFileType::kImage, std::nullopt));

  base::test::TestFuture<std::optional<std::vector<mojom::UploadedFilePtr>>>
      future;
  page_handler()->FinishUpload(future.GetCallback(),
                               std::make_optional(std::move(files)));

  auto result = future.Take();
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 2u);
  EXPECT_EQ((*result)[0]->filename, "report.pdf");
  EXPECT_EQ((*result)[1]->filename, "image.png");
}

TEST_F(AIChatUIPageHandlerTest, OnFilesUploaded_NonPdfGoesToFinish) {
  std::vector<mojom::UploadedFilePtr> files;
  files.push_back(mojom::UploadedFile::New(
      "/path/to/photo.png", 50, std::vector<uint8_t>(50),
      mojom::UploadedFileType::kImage, std::nullopt));

  base::test::TestFuture<std::optional<std::vector<mojom::UploadedFilePtr>>>
      future;
  page_handler()->OnFilesUploaded(future.GetCallback(),
                                  std::make_optional(std::move(files)));

  auto result = future.Take();
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 1u);
  EXPECT_EQ((*result)[0]->filename, "photo.png");
}

TEST_F(AIChatUIPageHandlerTest, OnAllPdfTextsExtracted_AppliesResults) {
  std::vector<mojom::UploadedFilePtr> files;
  files.push_back(
      mojom::UploadedFile::New("/path/doc1.pdf", 100, std::vector<uint8_t>(100),
                               mojom::UploadedFileType::kPdf, std::nullopt));
  files.push_back(
      mojom::UploadedFile::New("/path/photo.png", 50, std::vector<uint8_t>(50),
                               mojom::UploadedFileType::kImage, std::nullopt));
  files.push_back(
      mojom::UploadedFile::New("/path/doc2.pdf", 200, std::vector<uint8_t>(200),
                               mojom::UploadedFileType::kPdf, std::nullopt));

  std::vector<std::pair<size_t, std::optional<std::string>>> results;
  results.emplace_back(0, "Text from doc1");
  results.emplace_back(2, std::nullopt);  // extraction failed for doc2

  base::test::TestFuture<std::optional<std::vector<mojom::UploadedFilePtr>>>
      future;
  page_handler()->OnAllPdfTextsExtracted(future.GetCallback(),
                                         std::make_optional(std::move(files)),
                                         std::move(results));

  auto result = future.Take();
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 3u);
  // PDF extracted text applied and path stripped
  EXPECT_EQ((*result)[0]->filename, "doc1.pdf");
  ASSERT_TRUE((*result)[0]->extracted_text.has_value());
  EXPECT_EQ(*(*result)[0]->extracted_text, "Text from doc1");
  // Non-PDF unaffected, path stripped
  EXPECT_EQ((*result)[1]->filename, "photo.png");
  EXPECT_FALSE((*result)[1]->extracted_text.has_value());
  // Failed extraction, path stripped
  EXPECT_EQ((*result)[2]->filename, "doc2.pdf");
  EXPECT_FALSE((*result)[2]->extracted_text.has_value());
}

#if !BUILDFLAG(IS_ANDROID)
TEST_F(AIChatUIPageHandlerTest, OnAllTextFilesExtracted_AppliesResults) {
  std::vector<mojom::UploadedFilePtr> files;
  files.push_back(mojom::UploadedFile::New(
      "/path/config.conf", 100, std::vector<uint8_t>(100),
      mojom::UploadedFileType::kText, std::nullopt));
  files.push_back(
      mojom::UploadedFile::New("/path/photo.png", 50, std::vector<uint8_t>(50),
                               mojom::UploadedFileType::kImage, std::nullopt));
  files.push_back(mojom::UploadedFile::New(
      "/path/script.py", 200, std::vector<uint8_t>(200),
      mojom::UploadedFileType::kText, std::nullopt));

  std::vector<std::pair<size_t, std::optional<std::string>>> results;
  results.emplace_back(0, "key=value");
  results.emplace_back(2, std::nullopt);  // extraction failed for script.py

  base::test::TestFuture<std::optional<std::vector<mojom::UploadedFilePtr>>>
      future;
  page_handler()->OnAllTextFilesExtracted(future.GetCallback(),
                                          std::make_optional(std::move(files)),
                                          std::move(results));

  auto result = future.Take();
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 3u);
  // Text extracted and path stripped
  EXPECT_EQ((*result)[0]->filename, "config.conf");
  ASSERT_TRUE((*result)[0]->extracted_text.has_value());
  EXPECT_EQ(*(*result)[0]->extracted_text, "key=value");
  // Non-text unaffected, path stripped
  EXPECT_EQ((*result)[1]->filename, "photo.png");
  EXPECT_FALSE((*result)[1]->extracted_text.has_value());
  // Failed extraction, path stripped
  EXPECT_EQ((*result)[2]->filename, "script.py");
  EXPECT_FALSE((*result)[2]->extracted_text.has_value());
}
#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace ai_chat
