// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/renderer/page_content_extractor.h"

#include <memory>
#include <optional>
#include <string>

#include "base/test/test_future.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/test/render_view_test.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"

namespace ai_chat {

class PageContentExtractorRenderViewTest : public content::RenderViewTest {
 public:
  PageContentExtractorRenderViewTest() = default;
  ~PageContentExtractorRenderViewTest() override = default;

  void SetUp() override {
    content::RenderViewTest::SetUp();
    registry_ = std::make_unique<service_manager::BinderRegistry>();
  }

  void TearDown() override {
    registry_.reset();
    content::RenderViewTest::TearDown();
  }

  // Helper method to simulate loading a page with a specific URL
  void LoadPageWithUrl(const std::string& url) {
    LoadHTMLWithUrlOverride("<html><body>Test page</body></html>", url);
  }

  // Helper method to execute JavaScript and get the result
  std::optional<base::Value> ExecuteScript(const std::string& script) {
    blink::WebScriptSource source(blink::WebString::FromUTF8(script));
    base::test::TestFuture<std::optional<base::Value>, base::TimeTicks> future;

    GetMainRenderFrame()->GetWebFrame()->RequestExecuteScript(
        0, base::span_from_ref(source),
        blink::mojom::UserActivationOption::kDoNotActivate,
        blink::mojom::EvaluationTiming::kSynchronous,
        blink::mojom::LoadEventBlockingOption::kDoNotBlock,
        future.GetCallback(), blink::BackForwardCacheAware::kAllow,
        blink::mojom::WantResultOption::kWantResult,
        blink::mojom::PromiseResultOption::kAwait);

    auto [result, start_time] = future.Take();
    return std::move(result);
  }

  // Helper method to test ExtractPageContent through mojom interface
  mojom::PageContentPtr ExtractPageContent() {
    // Create the extractor instance to register the interface
    auto extractor = std::make_unique<PageContentExtractor>(
        GetMainRenderFrame(), registry_.get(), 0, 1);
    if (!extractor) {
      return nullptr;
    }

    base::test::TestFuture<mojom::PageContentPtr> future;

    extractor->ExtractPageContent(future.GetCallback());

    return future.Take();
  }

 private:
  std::unique_ptr<service_manager::BinderRegistry> registry_;
};

// Test ExtractPageContent for YouTube with ytcfg
TEST_F(PageContentExtractorRenderViewTest, ExtractPageContentYouTubeWithYtcfg) {
  LoadPageWithUrl("https://www.youtube.com/watch?v=test123");

  // Mock the ytcfg object and API key
  std::string setup_script = R"JS(
    window.ytcfg = {
      data_: {
        INNERTUBE_API_KEY: "test_api_key_123"
      }
    };
  )JS";

  ExecuteScript(setup_script);

  // Test ExtractPageContent
  auto result = ExtractPageContent();

  ASSERT_TRUE(result);
  EXPECT_EQ(result->type, mojom::PageContentType::VideoTranscriptYouTube);
  EXPECT_TRUE(result->content->is_youtube_inner_tube_config());

  const auto& config = result->content->get_youtube_inner_tube_config();
  EXPECT_EQ(config->api_key, "test_api_key_123");
  EXPECT_EQ(config->video_id, "test123");
}

// Test ExtractPageContent for YouTube with fallback regex
TEST_F(PageContentExtractorRenderViewTest, ExtractPageContentYouTubeFallback) {
  LoadPageWithUrl("https://www.youtube.com/watch?v=test456");

  // Mock script tag with API key instead of ytcfg
  std::string setup_script = R"JS(
    const script = document.createElement('script');
    script.textContent = '{"INNERTUBE_API_KEY":"fallback_api_key_456"}';
    document.head.appendChild(script);
  )JS";

  ExecuteScript(setup_script);

  // Test ExtractPageContent
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

  // Don't set up any ytcfg or script tags - should return null
  auto result = ExtractPageContent();

  // Should return null when API key is not found
  EXPECT_FALSE(result);
}

// Test ExtractPageContent for YouTube with missing video ID
TEST_F(PageContentExtractorRenderViewTest,
       ExtractPageContentYouTubeMissingVideoId) {
  LoadPageWithUrl("https://www.youtube.com/");

  // Mock the ytcfg object but no video ID in URL
  std::string setup_script = R"JS(
    window.ytcfg = {
      data_: {
        INNERTUBE_API_KEY: "test_api_key_789"
      }
    };
  )JS";

  ExecuteScript(setup_script);

  // Test ExtractPageContent
  auto result = ExtractPageContent();

  // Should return null when video ID is not found
  EXPECT_FALSE(result);
}

// Test ExtractPageContent for non-YouTube site (should fall back to text
// extraction)
TEST_F(PageContentExtractorRenderViewTest, ExtractPageContentNonYouTube) {
  LoadPageWithUrl("https://example.com");

  // Test ExtractPageContent
  auto result = ExtractPageContent();

  // For non-YouTube sites, it should attempt text extraction
  // The result might be null if no text content is found, or it might contain
  // text This test verifies the method doesn't crash and handles non-YouTube
  // sites The actual result depends on the text distillation logic
  EXPECT_TRUE(!result || result->type == mojom::PageContentType::Text);
}

}  // namespace ai_chat
