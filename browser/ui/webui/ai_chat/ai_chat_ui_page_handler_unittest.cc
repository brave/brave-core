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
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/test/web_contents_tester.h"
#include "pdf/buildflags.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/image/image_unittest_util.h"
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

TEST_F(AIChatUIPageHandlerTest, ProcessImageFile) {
  data_decoder::test::InProcessDataDecoder data_decoder;

  // Empty data.
  {
    base::test::TestFuture<mojom::UploadedFilePtr> future;
    page_handler()->ProcessImageFile({}, "empty.png", future.GetCallback());
    EXPECT_FALSE(future.Take());
  }

  // Invalid (non-image) data.
  {
    base::test::TestFuture<mojom::UploadedFilePtr> future;
    page_handler()->ProcessImageFile({1, 2, 3, 4}, "invalid.png",
                                     future.GetCallback());
    EXPECT_FALSE(future.Take());
  }

  // Valid 1x1 PNG - should succeed and round-trip correctly.
  constexpr uint8_t kSamplePng[] = {
      0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00,
      0x0d, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
      0x00, 0x01, 0x08, 0x02, 0x00, 0x00, 0x00, 0x90, 0x77, 0x53, 0xde,
      0x00, 0x00, 0x00, 0x10, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x62,
      0x5a, 0xc4, 0x5e, 0x08, 0x08, 0x00, 0x00, 0xff, 0xff, 0x02, 0x71,
      0x01, 0x1d, 0xcd, 0xd0, 0xd6, 0x62, 0x00, 0x00, 0x00, 0x00, 0x49,
      0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82};
  auto sample_bitmap = gfx::PNGCodec::Decode(kSamplePng);
  {
    std::vector<uint8_t> data(std::begin(kSamplePng), std::end(kSamplePng));
    base::test::TestFuture<mojom::UploadedFilePtr> future;
    page_handler()->ProcessImageFile(data, "sample.png", future.GetCallback());
    auto result = future.Take();
    ASSERT_TRUE(result);
    EXPECT_EQ(result->filename, "sample.png");
    EXPECT_EQ(result->type, mojom::UploadedFileType::kImage);
    EXPECT_GT(result->data.size(), 0u);
    EXPECT_EQ(result->filesize, result->data.size());
    auto decoded = gfx::PNGCodec::Decode(result->data);
    EXPECT_EQ(decoded.width(), sample_bitmap.width());
    EXPECT_EQ(decoded.height(), sample_bitmap.height());
  }

  // Large PNG (2048x2048) - should be scaled down to fit within 1024x768.
  {
    auto large_png_bytes = gfx::test::CreatePNGBytes(2048);
    std::vector<uint8_t> data(large_png_bytes->begin(), large_png_bytes->end());
    base::test::TestFuture<mojom::UploadedFilePtr> future;
    page_handler()->ProcessImageFile(data, "large.png", future.GetCallback());
    auto result = future.Take();
    ASSERT_TRUE(result);
    EXPECT_EQ(result->filename, "large.png");
    EXPECT_EQ(result->type, mojom::UploadedFileType::kImage);
    EXPECT_GT(result->data.size(), 0u);
    auto decoded = gfx::PNGCodec::Decode(result->data);
    EXPECT_EQ(decoded.width(), 1024);
    EXPECT_EQ(decoded.height(), 768);
  }
}

#if BUILDFLAG(ENABLE_PDF)
TEST_F(AIChatUIPageHandlerTest, ProcessPdfFile_LooksLikePdfRejection) {
  // Data shorter than 50 bytes, even with a valid PDF header.
  {
    constexpr std::string_view kPdfHeader = "%PDF-1.4";
    std::vector<uint8_t> too_small(kPdfHeader.begin(), kPdfHeader.end());
    base::test::TestFuture<mojom::UploadedFilePtr> future;
    page_handler()->ProcessPdfFile(too_small, "small.pdf",
                                   future.GetCallback());
    EXPECT_FALSE(future.Take());
  }

  // Sufficient size (>= 50 bytes) but wrong header.
  {
    std::vector<uint8_t> wrong_header(50, 'x');
    base::test::TestFuture<mojom::UploadedFilePtr> future;
    page_handler()->ProcessPdfFile(wrong_header, "fake.pdf",
                                   future.GetCallback());
    EXPECT_FALSE(future.Take());
  }
}
#endif  // BUILDFLAG(ENABLE_PDF)

}  // namespace ai_chat
