// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/renderer/page_content_extractor.h"

#include <memory>
#include <string>

#include "base/test/test_future.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/test/render_view_test.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class PageContentExtractorRenderViewTest : public content::RenderViewTest {
 public:
  PageContentExtractorRenderViewTest() = default;
  ~PageContentExtractorRenderViewTest() override = default;

  void SetUp() override {
    content::RenderViewTest::SetUp();
    registry_ = std::make_unique<service_manager::BinderRegistry>();

    // Create the PageContentExtractor
    extractor_ = std::make_unique<PageContentExtractor>(GetMainRenderFrame(),
                                                        registry_.get(), 0, 1);
  }

  void TearDown() override {
    extractor_.reset();
    registry_.reset();
    content::RenderViewTest::TearDown();
  }

 protected:
  void LoadPageWithUrl(std::string_view url,
                       const std::string& script_content = "") {
    std::string html = "<html><head>";
    if (!script_content.empty()) {
      html += "<script>" + script_content + "</script>";
    }
    html += "</head><body></body></html>";
    LoadHTMLWithUrlOverride(html, url);
  }

  mojom::PageContentPtr ExtractPageContent() {
    base::test::TestFuture<mojom::PageContentPtr> future;

    extractor_->ExtractPageContent(future.GetCallback());

    return future.Take();
  }

 private:
  std::unique_ptr<service_manager::BinderRegistry> registry_;
  std::unique_ptr<PageContentExtractor> extractor_;
};

// Test ExtractPageContent for YouTube with ytcfg
TEST_F(PageContentExtractorRenderViewTest, ExtractPageContentYouTubeWithYtcfg) {
  // Mock the ytcfg object and API key
  constexpr char kScript[] = R"JS(
    window.ytcfg = {
      data_: {
        INNERTUBE_API_KEY: "test_api_key_123"
      }
    };
  )JS";

  LoadPageWithUrl("https://www.youtube.com/watch?v=test123", kScript);

  auto result = ExtractPageContent();

  ASSERT_TRUE(result);
  EXPECT_EQ(result->type, mojom::PageContentType::VideoTranscriptYouTube);
  EXPECT_TRUE(result->content->is_youtube_inner_tube_config());

  const auto& config = result->content->get_youtube_inner_tube_config();
  EXPECT_EQ(config->api_key, "test_api_key_123");
  EXPECT_EQ(config->video_id, "test123");
}

// Test ExtractPageContent for YouTube with fallback regex
TEST_F(PageContentExtractorRenderViewTest,
       ExtractPageContentYouTubeWithFallback) {
  // Mock script tag with API key instead of ytcfg
  constexpr char kScript[] = R"JS(
    var script = document.createElement('script');
    script.textContent = '"INNERTUBE_API_KEY":"fallback_api_key_456"';
    document.head.appendChild(script);
  )JS";

  LoadPageWithUrl("https://www.youtube.com/watch?v=test456", kScript);

  auto result = ExtractPageContent();

  ASSERT_TRUE(result);
  EXPECT_EQ(result->type, mojom::PageContentType::VideoTranscriptYouTube);
  EXPECT_TRUE(result->content->is_youtube_inner_tube_config());

  const auto& config = result->content->get_youtube_inner_tube_config();
  EXPECT_EQ(config->api_key, "fallback_api_key_456");
  EXPECT_EQ(config->video_id, "test456");
}

// Test ExtractPageContent for YouTube with missing data
TEST_F(PageContentExtractorRenderViewTest,
       ExtractPageContentYouTubeMissingData) {
  LoadPageWithUrl("https://www.youtube.com/watch?v=test789");

  auto result = ExtractPageContent();

  // Should return null when API key is not found
  EXPECT_FALSE(result);
}

// Test ExtractPageContent for YouTube with missing video ID
TEST_F(PageContentExtractorRenderViewTest,
       ExtractPageContentYouTubeMissingVideoId) {
  // Mock the ytcfg object but no video ID in URL
  constexpr char kScript[] = R"JS(
    window.ytcfg = {
      data_: {
        INNERTUBE_API_KEY: "test_api_key_789"
      }
    };
  )JS";

  LoadPageWithUrl("https://www.youtube.com/", kScript);

  auto result = ExtractPageContent();

  // Should return null when video ID is not found
  EXPECT_FALSE(result);
}

// Test ExtractPageContent for non-YouTube site (should fall back to text
// extraction)
TEST_F(PageContentExtractorRenderViewTest, ExtractPageContentNonYouTube) {
  LoadPageWithUrl("https://example.com");

  auto result = ExtractPageContent();

  // For non-YouTube sites, it should attempt text extraction
  // The result might be null if no text content is found, or it might contain
  // text This test verifies the method doesn't crash and handles non-YouTube
  // sites The actual result depends on the text distillation logic
  EXPECT_TRUE(!result || result->type == mojom::PageContentType::Text);
}

}  // namespace ai_chat
