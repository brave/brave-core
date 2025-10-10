// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/page_content_fetcher.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/test_renderer_host.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/network/test/test_url_loader_factory.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "url/gurl.h"

using testing::_;
using testing::Invoke;

namespace ai_chat {

namespace {

constexpr char kGithubPatch[] = R"(diff --git a/file.cc b/file.cc
index 9e2e7d6ef96..4cdf7cc8ac8 100644
--- a/file.cc
+++ b/file.cc
@@ -7,6 +7,7 @@
 #include "file3.h"
 #include "file4.h"
+
+int main() {
+    std::cout << "This is the way" << std::endl;
+    return 0;
+})";

constexpr char kGithubRawFile[] = R"(// Sample file content
#include <iostream>

int main() {
  std::cout << "Hello World" << std::endl;
  return 0;
})";

constexpr char kGithubAtomFeed[] = R"(<?xml version="1.0" encoding="UTF-8"?>
<feed xmlns="http://www.w3.org/2005/Atom">
  <title>Recent Commits to din_djarin:master</title>
  <entry>
    <id>tag:github.com,2008:Grit::Commit/abc123</id>
    <title>Add new feature</title>
    <updated>2025-01-15T10:00:00Z</updated>
  </entry>
</feed>)";

// Mock PageContentExtractor for testing
class MockPageContentExtractor : public mojom::PageContentExtractor {
 public:
  MockPageContentExtractor() = default;
  ~MockPageContentExtractor() override = default;

  MOCK_METHOD(void,
              ExtractPageContent,
              (ExtractPageContentCallback),
              (override));
  MOCK_METHOD(void,
              GetSearchSummarizerKey,
              (GetSearchSummarizerKeyCallback),
              (override));
  MOCK_METHOD(void,
              GetOpenAIChatButtonNonce,
              (GetOpenAIChatButtonNonceCallback),
              (override));

  void Bind(mojo::ScopedMessagePipeHandle handle) {
    receiver_.Bind(
        mojo::PendingReceiver<mojom::PageContentExtractor>(std::move(handle)));
  }

 private:
  mojo::Receiver<mojom::PageContentExtractor> receiver_{this};
};

// Helper class to create PageContent with YouTube InnerTube config
mojom::PageContentPtr CreateYoutubePageContent(const std::string& api_key,
                                               const std::string& video_id) {
  auto page_content = mojom::PageContent::New();
  page_content->type = mojom::PageContentType::VideoTranscriptYouTube;

  auto config = mojom::YoutubeInnerTubeConfig::New();
  config->api_key = api_key;
  config->video_id = video_id;
  page_content->content =
      mojom::PageContentData::NewYoutubeInnerTubeConfig(std::move(config));

  return page_content;
}

// Helper class to create PageContent with text content
mojom::PageContentPtr CreateTextPageContent(const std::string& text) {
  auto page_content = mojom::PageContent::New();
  page_content->type = mojom::PageContentType::Text;
  page_content->content = mojom::PageContentData::NewContent(text);
  return page_content;
}

// Helper class to create PageContent with content URL
mojom::PageContentPtr CreateContentUrlPageContent(const GURL& url) {
  auto page_content = mojom::PageContent::New();
  page_content->type = mojom::PageContentType::VideoTranscriptVTT;
  page_content->content = mojom::PageContentData::NewContentUrl(url);
  return page_content;
}

}  // namespace

class PageContentFetcherTest : public content::RenderViewHostTestHarness {
 public:
  PageContentFetcherTest() = default;
  ~PageContentFetcherTest() override = default;

  void SetUp() override {
    content::RenderViewHostTestHarness::SetUp();
    test_url_loader_factory_ =
        std::make_unique<network::TestURLLoaderFactory>();
    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            test_url_loader_factory_.get());

    fetcher_ = std::make_unique<PageContentFetcher>(web_contents());
    fetcher_->SetURLLoaderFactoryForTesting(shared_url_loader_factory_);
  }

  void TearDown() override {
    fetcher_.reset();
    shared_url_loader_factory_.reset();
    test_url_loader_factory_.reset();
    content::RenderViewHostTestHarness::TearDown();
  }

 protected:
  // Helper method to set up mock PageContentExtractor
  MockPageContentExtractor* SetUpMockExtractor() {
    content::RenderFrameHostTester::For(web_contents()->GetPrimaryMainFrame())
        ->InitializeRenderFrameIfNeeded();
    std::unique_ptr<service_manager::InterfaceProvider::TestApi> test_api =
        std::make_unique<service_manager::InterfaceProvider::TestApi>(
            web_contents()->GetPrimaryMainFrame()->GetRemoteInterfaces());

    mock_extractor_ = std::make_unique<MockPageContentExtractor>();
    MockPageContentExtractor* extractor_ptr = mock_extractor_.get();

    test_api->SetBinderForName(
        mojom::PageContentExtractor::Name_,
        base::BindRepeating(&MockPageContentExtractor::Bind,
                            base::Unretained(extractor_ptr)));

    return extractor_ptr;
  }

  // Helper method to simulate a network response
  void SimulateNetworkResponse(const GURL& url,
                               net::HttpStatusCode status_code,
                               const std::string& response_body) {
    auto response_head = network::mojom::URLResponseHead::New();
    response_head->headers = base::MakeRefCounted<net::HttpResponseHeaders>(
        "HTTP/1.1 " + base::NumberToString(status_code) + " OK");
    response_head->headers->AddHeader("Content-Type", "application/json");

    test_url_loader_factory_->AddResponse(url, std::move(response_head),
                                          response_body,
                                          network::URLLoaderCompletionStatus());
  }

  // Helper method to simulate a network error
  void SimulateNetworkError(const GURL& url, net::Error error) {
    test_url_loader_factory_->AddResponse(
        url, network::mojom::URLResponseHead::New(), "",
        network::URLLoaderCompletionStatus(error));
  }

  // Helper method to create a YouTube InnerTube API response
  std::string CreateInnerTubeResponse(const std::string& base_url) {
    base::Value response = base::test::ParseJson(absl::StrFormat(R"({
      "captions": {
        "playerCaptionsTracklistRenderer": {
          "captionTracks": [
            {
              "baseUrl": "%s",
              "name": "English",
              "languageCode": "en"
            }
          ]
        }
      }
    })",
                                                                 base_url));

    std::string response_str;
    base::JSONWriter::Write(response, &response_str);
    return response_str;
  }

  // Helper method to create a YouTube transcript XML response using <timedtext>
  // format
  std::string CreateTranscriptXmlResponse(const std::string& transcript_text) {
    return R"(<?xml version="1.0" encoding="utf-8"?>
<timedtext format="3">
  <body>
    <p t="0" d="5000">)" +
           transcript_text + R"(</p>
  </body>
</timedtext>)";
  }

  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
  std::unique_ptr<network::TestURLLoaderFactory> test_url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<PageContentFetcher> fetcher_;
  std::unique_ptr<MockPageContentExtractor> mock_extractor_;
};

// Test successful YouTube InnerTube API request flow
TEST_F(PageContentFetcherTest, YouTubeInnerTubeAPISuccess) {
  const std::string api_key = "test_api_key_123";
  const std::string video_id = "test_video_456";
  const std::string transcript_text = "This is a test transcript";

  NavigateAndCommit(GURL("https://www.youtube.com/watch?v=" + video_id));

  // Set up mock extractor that returns YouTube InnerTube config
  MockPageContentExtractor* mock_extractor = SetUpMockExtractor();
  auto youtube_content = CreateYoutubePageContent(api_key, video_id);

  EXPECT_CALL(*mock_extractor, ExtractPageContent(_))
      .WillOnce([&youtube_content](
                    mojom::PageContentExtractor::ExtractPageContentCallback
                        callback) {
        std::move(callback).Run(youtube_content.Clone());
      });

  // Set up network responses
  GURL inner_tube_url("https://www.youtube.com/youtubei/v1/player?key=" +
                      api_key);
  GURL transcript_url("https://www.youtube.com/api/timedtext?lang=en&v=" +
                      video_id);

  // Track the requests that are made
  std::vector<network::ResourceRequest> requests_made;
  std::vector<std::string> request_bodies;
  test_url_loader_factory_->SetInterceptor(
      base::BindLambdaForTesting([&requests_made, &request_bodies](
                                     const network::ResourceRequest& request) {
        requests_made.push_back(request);
        // Capture the request body if it exists
        if (request.request_body) {
          const auto* elements = request.request_body->elements();
          if (elements && !elements->empty() &&
              elements->at(0).type() == network::DataElement::Tag::kBytes) {
            request_bodies.push_back(
                std::string(elements->at(0)
                                .As<network::DataElementBytes>()
                                .AsStringPiece()));
          }
        }
      }));

  // Mock InnerTube API response
  std::string inner_tube_response =
      CreateInnerTubeResponse(transcript_url.spec());
  SimulateNetworkResponse(inner_tube_url, net::HTTP_OK, inner_tube_response);

  // Mock transcript XML response using <timedtext> format
  std::string transcript_xml = CreateTranscriptXmlResponse(transcript_text);
  SimulateNetworkResponse(transcript_url, net::HTTP_OK, transcript_xml);

  base::test::TestFuture<std::string, bool, std::string> future;
  fetcher_->FetchPageContent("", future.GetCallback());

  // Wait for the result
  auto [content, is_video, invalidation_token] = future.Get();

  EXPECT_EQ(content, transcript_text);
  EXPECT_TRUE(is_video);
  EXPECT_FALSE(invalidation_token.empty());

  // Verify that the correct requests were made
  EXPECT_EQ(requests_made.size(), 2u);

  // Check InnerTube API request
  EXPECT_EQ(requests_made[0].url, inner_tube_url);
  EXPECT_EQ(requests_made[0].method, "POST");
  EXPECT_EQ(requests_made[0].headers.GetHeader("Content-Type"),
            "application/json");

  // Verify the request body
  ASSERT_EQ(request_bodies.size(), 1u);
  base::Value expected_body = base::test::ParseJson(absl::StrFormat(R"({
    "videoId": "%s",
    "context": {
      "client": {
        "clientName": "ANDROID",
        "clientVersion": "20.10.38"
      }
    }
  })",
                                                                    video_id));

  std::string expected_body_str;
  base::JSONWriter::Write(expected_body, &expected_body_str);
  EXPECT_EQ(request_bodies[0], expected_body_str);

  // Check transcript request
  EXPECT_EQ(requests_made[1].url, transcript_url);
  EXPECT_EQ(requests_made[1].method, "GET");
}

// Test YouTube InnerTube API request with network error
TEST_F(PageContentFetcherTest, YouTubeInnerTubeAPINetworkError) {
  const std::string api_key = "test_api_key_123";
  const std::string video_id = "test_video_456";

  NavigateAndCommit(GURL("https://www.youtube.com/watch?v=" + video_id));

  // Set up mock extractor that returns YouTube InnerTube config
  MockPageContentExtractor* mock_extractor = SetUpMockExtractor();
  auto youtube_content = CreateYoutubePageContent(api_key, video_id);

  EXPECT_CALL(*mock_extractor, ExtractPageContent(_))
      .WillOnce([&youtube_content](
                    mojom::PageContentExtractor::ExtractPageContentCallback
                        callback) {
        std::move(callback).Run(youtube_content.Clone());
      });

  // Track the requests that are made
  std::vector<network::ResourceRequest> requests_made;
  test_url_loader_factory_->SetInterceptor(base::BindLambdaForTesting(
      [&requests_made](const network::ResourceRequest& request) {
        requests_made.push_back(request);
      }));

  // Set up network error for InnerTube API
  GURL inner_tube_url("https://www.youtube.com/youtubei/v1/player?key=" +
                      api_key);
  SimulateNetworkError(inner_tube_url, net::ERR_NETWORK_CHANGED);

  base::test::TestFuture<std::string, bool, std::string> future;
  fetcher_->FetchPageContent("", future.GetCallback());

  // Wait for the result
  auto [content, is_video, invalidation_token] = future.Get();

  EXPECT_TRUE(content.empty());
  EXPECT_TRUE(is_video);
  EXPECT_FALSE(invalidation_token.empty());

  // Verify that the InnerTube API request was made
  EXPECT_EQ(requests_made.size(), 1u);
  EXPECT_EQ(requests_made[0].url, inner_tube_url);
  EXPECT_EQ(requests_made[0].method, "POST");
}

// Test YouTube InnerTube API request with invalid JSON response
TEST_F(PageContentFetcherTest, YouTubeInnerTubeAPIInvalidJsonResponse) {
  const std::string api_key = "test_api_key_123";
  const std::string video_id = "test_video_456";

  NavigateAndCommit(GURL("https://www.youtube.com/watch?v=" + video_id));

  // Set up mock extractor that returns YouTube InnerTube config
  MockPageContentExtractor* mock_extractor = SetUpMockExtractor();
  auto youtube_content = CreateYoutubePageContent(api_key, video_id);

  EXPECT_CALL(*mock_extractor, ExtractPageContent(_))
      .WillOnce([&youtube_content](
                    mojom::PageContentExtractor::ExtractPageContentCallback
                        callback) {
        std::move(callback).Run(youtube_content.Clone());
      });

  // Track the requests that are made
  std::vector<network::ResourceRequest> requests_made;
  test_url_loader_factory_->SetInterceptor(base::BindLambdaForTesting(
      [&requests_made](const network::ResourceRequest& request) {
        requests_made.push_back(request);
      }));

  // Set up invalid JSON response for InnerTube API
  GURL inner_tube_url("https://www.youtube.com/youtubei/v1/player?key=" +
                      api_key);
  SimulateNetworkResponse(inner_tube_url, net::HTTP_OK, "invalid json");

  base::test::TestFuture<std::string, bool, std::string> future;
  fetcher_->FetchPageContent("", future.GetCallback());

  // Wait for the result
  auto [content, is_video, invalidation_token] = future.Get();

  EXPECT_TRUE(content.empty());
  EXPECT_TRUE(is_video);
  EXPECT_FALSE(invalidation_token.empty());

  // Verify that the InnerTube API request was made
  EXPECT_EQ(requests_made.size(), 1u);
  EXPECT_EQ(requests_made[0].url, inner_tube_url);
  EXPECT_EQ(requests_made[0].method, "POST");
}

// Test YouTube InnerTube API request with missing caption tracks
TEST_F(PageContentFetcherTest, YouTubeInnerTubeAPINoCaptionTracks) {
  const std::string api_key = "test_api_key_123";
  const std::string video_id = "test_video_456";

  NavigateAndCommit(GURL("https://www.youtube.com/watch?v=" + video_id));

  // Set up mock extractor that returns YouTube InnerTube config
  MockPageContentExtractor* mock_extractor = SetUpMockExtractor();
  auto youtube_content = CreateYoutubePageContent(api_key, video_id);

  EXPECT_CALL(*mock_extractor, ExtractPageContent(_))
      .WillOnce([&youtube_content](
                    mojom::PageContentExtractor::ExtractPageContentCallback
                        callback) {
        std::move(callback).Run(youtube_content.Clone());
      });

  // Track the requests that are made
  std::vector<network::ResourceRequest> requests_made;
  test_url_loader_factory_->SetInterceptor(base::BindLambdaForTesting(
      [&requests_made](const network::ResourceRequest& request) {
        requests_made.push_back(request);
      }));

  // Set up response without caption tracks
  GURL inner_tube_url("https://www.youtube.com/youtubei/v1/player?key=" +
                      api_key);
  SimulateNetworkResponse(inner_tube_url, net::HTTP_OK, "{}");

  base::test::TestFuture<std::string, bool, std::string> future;
  fetcher_->FetchPageContent("", future.GetCallback());

  // Wait for the result
  auto [content, is_video, invalidation_token] = future.Get();

  EXPECT_TRUE(content.empty());
  EXPECT_TRUE(is_video);
  EXPECT_FALSE(invalidation_token.empty());

  // Verify that the InnerTube API request was made
  EXPECT_EQ(requests_made.size(), 1u);
  EXPECT_EQ(requests_made[0].url, inner_tube_url);
  EXPECT_EQ(requests_made[0].method, "POST");
}

// Test invalidation token caching
TEST_F(PageContentFetcherTest, InvalidationTokenCaching) {
  const std::string api_key = "test_api_key_123";
  const std::string video_id = "test_video_456";
  const std::string transcript_text = "This is a test transcript";

  NavigateAndCommit(GURL("https://www.youtube.com/watch?v=" + video_id));

  // Set up mock extractor that returns YouTube InnerTube config
  MockPageContentExtractor* mock_extractor = SetUpMockExtractor();
  auto youtube_content = CreateYoutubePageContent(api_key, video_id);

  EXPECT_CALL(*mock_extractor, ExtractPageContent(_))
      .WillOnce([&youtube_content](
                    mojom::PageContentExtractor::ExtractPageContentCallback
                        callback) {
        std::move(callback).Run(youtube_content.Clone());
      });

  // Track the requests that are made
  std::vector<network::ResourceRequest> requests_made;
  test_url_loader_factory_->SetInterceptor(base::BindLambdaForTesting(
      [&requests_made](const network::ResourceRequest& request) {
        requests_made.push_back(request);
      }));

  // Set up network responses
  GURL inner_tube_url("https://www.youtube.com/youtubei/v1/player?key=" +
                      api_key);
  GURL transcript_url("https://www.youtube.com/api/timedtext?lang=en&v=" +
                      video_id);

  std::string inner_tube_response =
      CreateInnerTubeResponse(transcript_url.spec());
  SimulateNetworkResponse(inner_tube_url, net::HTTP_OK, inner_tube_response);
  std::string transcript_xml = CreateTranscriptXmlResponse(transcript_text);
  SimulateNetworkResponse(transcript_url, net::HTTP_OK, transcript_xml);

  base::test::TestFuture<std::string, bool, std::string> future1;
  fetcher_->FetchPageContent("", future1.GetCallback());

  // Wait for the first result
  auto [content1, is_video1, invalidation_token1] = future1.Get();

  EXPECT_EQ(content1, transcript_text);
  EXPECT_TRUE(is_video1);
  EXPECT_FALSE(invalidation_token1.empty());
  EXPECT_EQ(requests_made.size(), 2u);

  // Reset the mock extractor for the second call
  MockPageContentExtractor* mock_extractor2 = SetUpMockExtractor();
  auto youtube_content2 = CreateYoutubePageContent(api_key, video_id);

  EXPECT_CALL(*mock_extractor2, ExtractPageContent(_))
      .WillOnce([&youtube_content2](
                    mojom::PageContentExtractor::ExtractPageContentCallback
                        callback) {
        std::move(callback).Run(youtube_content2.Clone());
      });

  base::test::TestFuture<std::string, bool, std::string> future2;
  fetcher_->FetchPageContent(invalidation_token1, future2.GetCallback());

  // Wait for the second result
  auto [content2, is_video2, invalidation_token2] = future2.Get();

  EXPECT_TRUE(content2.empty());  // Should return empty content when cached
  EXPECT_TRUE(is_video2);
  EXPECT_EQ(invalidation_token2, invalidation_token1);
  EXPECT_EQ(requests_made.size(), 2u);  // No new requests
}

// Test content URL extraction (non-YouTube video)
TEST_F(PageContentFetcherTest, ContentUrlExtraction) {
  const std::string transcript_text = "This is a VTT transcript";
  GURL content_url("https://example.com/transcript.vtt");

  NavigateAndCommit(GURL("https://example.com/video"));

  // Set up mock extractor that returns content URL
  MockPageContentExtractor* mock_extractor = SetUpMockExtractor();
  auto content_url_page_content = CreateContentUrlPageContent(content_url);

  EXPECT_CALL(*mock_extractor, ExtractPageContent(_))
      .WillOnce([&content_url_page_content](
                    mojom::PageContentExtractor::ExtractPageContentCallback
                        callback) {
        std::move(callback).Run(content_url_page_content.Clone());
      });

  // Track the requests that are made
  std::vector<network::ResourceRequest> requests_made;
  test_url_loader_factory_->SetInterceptor(base::BindLambdaForTesting(
      [&requests_made](const network::ResourceRequest& request) {
        requests_made.push_back(request);
      }));

  // Set up network response for content URL
  SimulateNetworkResponse(content_url, net::HTTP_OK, transcript_text);

  base::test::TestFuture<std::string, bool, std::string> future;
  fetcher_->FetchPageContent("", future.GetCallback());

  // Wait for the result
  auto [content, is_video, invalidation_token] = future.Get();

  EXPECT_EQ(content, transcript_text);
  EXPECT_TRUE(is_video);
  EXPECT_FALSE(invalidation_token.empty());
  EXPECT_EQ(requests_made.size(), 1u);

  // Verify the request was made to the correct URL
  EXPECT_EQ(requests_made[0].url, content_url);
  EXPECT_EQ(requests_made[0].method, "GET");
}

// Test null PageContent response
TEST_F(PageContentFetcherTest, NullPageContentResponse) {
  NavigateAndCommit(GURL("https://example.com"));

  // Set up mock extractor that returns null
  MockPageContentExtractor* mock_extractor = SetUpMockExtractor();

  EXPECT_CALL(*mock_extractor, ExtractPageContent(_))
      .WillOnce(
          [](mojom::PageContentExtractor::ExtractPageContentCallback callback) {
            std::move(callback).Run(nullptr);
          });

  base::test::TestFuture<std::string, bool, std::string> future;
  fetcher_->FetchPageContent("", future.GetCallback());

  // Wait for the result
  auto [content, is_video, invalidation_token] = future.Get();

  EXPECT_TRUE(content.empty());
  EXPECT_FALSE(is_video);
  EXPECT_TRUE(invalidation_token.empty());
  EXPECT_EQ(test_url_loader_factory_->total_requests(), 0u);
}

// Test text content extraction (non-video)
TEST_F(PageContentFetcherTest, TextContentExtraction) {
  const std::string text_content = "This is some text content from a webpage";

  NavigateAndCommit(GURL("https://example.com"));

  // Set up mock extractor that returns text content
  MockPageContentExtractor* mock_extractor = SetUpMockExtractor();
  auto text_page_content = CreateTextPageContent(text_content);

  EXPECT_CALL(*mock_extractor, ExtractPageContent(_))
      .WillOnce([&text_page_content](
                    mojom::PageContentExtractor::ExtractPageContentCallback
                        callback) {
        std::move(callback).Run(text_page_content.Clone());
      });

  base::test::TestFuture<std::string, bool, std::string> future;
  fetcher_->FetchPageContent("", future.GetCallback());

  // Wait for the result
  auto [content, is_video, invalidation_token] = future.Get();

  EXPECT_EQ(content, text_content);
  EXPECT_FALSE(is_video);
  EXPECT_TRUE(invalidation_token.empty());
  EXPECT_EQ(test_url_loader_factory_->total_requests(), 0u);
}

// Test that API keys with special characters are properly URL-encoded
TEST_F(PageContentFetcherTest, YouTubeInnerTubeAPIKeyUrlEncoding) {
  // API key with special characters that need URL encoding
  const std::string api_key_with_special_chars =
      "test+api_key=with&special?chars#123 space";
  const std::string video_id = "test_video_456";
  const std::string transcript_text = "This is a test transcript";

  NavigateAndCommit(GURL("https://www.youtube.com/watch?v=" + video_id));

  // Set up mock extractor that returns YouTube InnerTube config
  MockPageContentExtractor* mock_extractor = SetUpMockExtractor();
  auto youtube_content =
      CreateYoutubePageContent(api_key_with_special_chars, video_id);

  EXPECT_CALL(*mock_extractor, ExtractPageContent(_))
      .WillOnce([&youtube_content](
                    mojom::PageContentExtractor::ExtractPageContentCallback
                        callback) {
        std::move(callback).Run(youtube_content.Clone());
      });

  // Track the requests that are made
  std::vector<network::ResourceRequest> requests_made;
  test_url_loader_factory_->SetInterceptor(base::BindLambdaForTesting(
      [&requests_made](const network::ResourceRequest& request) {
        requests_made.push_back(request);
      }));

  // Calculate the expected URL with properly encoded API key
  // The original API key "test+api_key=with&special?chars#123 space" should be
  // encoded as "test%2Bapi_key%3Dwith%26special%3Fchars%23123+space" when using
  // EscapeQueryParamValue with use_plus=true (spaces become +)
  std::string expected_encoded_api_key =
      "test%2Bapi_key%3Dwith%26special%3Fchars%23123+space";
  GURL expected_inner_tube_url(
      "https://www.youtube.com/youtubei/v1/player?key=" +
      expected_encoded_api_key);
  GURL transcript_url("https://www.youtube.com/api/timedtext?lang=en&v=" +
                      video_id);

  // Mock InnerTube API response
  std::string inner_tube_response =
      CreateInnerTubeResponse(transcript_url.spec());
  SimulateNetworkResponse(expected_inner_tube_url, net::HTTP_OK,
                          inner_tube_response);

  // Mock transcript XML response
  std::string transcript_xml = CreateTranscriptXmlResponse(transcript_text);
  SimulateNetworkResponse(transcript_url, net::HTTP_OK, transcript_xml);

  base::test::TestFuture<std::string, bool, std::string> future;
  fetcher_->FetchPageContent("", future.GetCallback());

  // Wait for the result
  auto [content, is_video, invalidation_token] = future.Get();

  EXPECT_EQ(content, transcript_text);
  EXPECT_TRUE(is_video);
  EXPECT_FALSE(invalidation_token.empty());

  // Verify that the correct requests were made
  EXPECT_EQ(requests_made.size(), 2u);

  // Check that the InnerTube API request URL has the properly encoded API key
  EXPECT_EQ(requests_made[0].url, expected_inner_tube_url);
  EXPECT_EQ(requests_made[0].method, "POST");
  EXPECT_EQ(requests_made[0].headers.GetHeader("Content-Type"),
            "application/json");

  // Verify that the API key in the URL is properly encoded
  EXPECT_EQ(requests_made[0].url.spec(), expected_inner_tube_url.spec());

  // Check transcript request
  EXPECT_EQ(requests_made[1].url, transcript_url);
  EXPECT_EQ(requests_made[1].method, "GET");
}

// Test GitHub pull request URL fetching
TEST_F(PageContentFetcherTest, GithubPullRequestUrl) {
  GURL pr_url("https://github.com/brave/din_djarin/pull/65535");
  GURL expected_patch_url(
      "https://github.com/brave/din_djarin/pull/65535.patch");

  NavigateAndCommit(pr_url);

  // Track the requests that are made
  std::vector<network::ResourceRequest> requests_made;
  test_url_loader_factory_->SetInterceptor(base::BindLambdaForTesting(
      [&requests_made](const network::ResourceRequest& request) {
        requests_made.push_back(request);
      }));

  // Set up network response for patch URL
  SimulateNetworkResponse(expected_patch_url, net::HTTP_OK, kGithubPatch);

  base::test::TestFuture<std::string, bool, std::string> future;
  fetcher_->FetchPageContent("", future.GetCallback());

  // Wait for the result
  auto [content, is_video, invalidation_token] = future.Get();

  EXPECT_EQ(content, kGithubPatch);
  EXPECT_FALSE(is_video);
  EXPECT_TRUE(invalidation_token.empty());

  // Verify the request was made to the patch URL
  EXPECT_EQ(requests_made.size(), 1u);
  EXPECT_EQ(requests_made[0].url, expected_patch_url);
  EXPECT_EQ(requests_made[0].method, "GET");
}

// Test GitHub commit URL fetching
TEST_F(PageContentFetcherTest, GithubCommitUrl) {
  GURL commit_url(
      "https://github.com/brave/din_djarin/commit/"
      "64bdda5969698bf570002b9f99852f5f595c2e3c");
  GURL expected_patch_url(
      "https://github.com/brave/din_djarin/commit/"
      "64bdda5969698bf570002b9f99852f5f595c2e3c.patch");

  NavigateAndCommit(commit_url);

  // Track the requests that are made
  std::vector<network::ResourceRequest> requests_made;
  test_url_loader_factory_->SetInterceptor(base::BindLambdaForTesting(
      [&requests_made](const network::ResourceRequest& request) {
        requests_made.push_back(request);
      }));

  // Set up network response for patch URL
  SimulateNetworkResponse(expected_patch_url, net::HTTP_OK, kGithubPatch);

  base::test::TestFuture<std::string, bool, std::string> future;
  fetcher_->FetchPageContent("", future.GetCallback());

  // Wait for the result
  auto [content, is_video, invalidation_token] = future.Get();

  EXPECT_EQ(content, kGithubPatch);
  EXPECT_FALSE(is_video);
  EXPECT_TRUE(invalidation_token.empty());

  // Verify the request was made to the patch URL
  EXPECT_EQ(requests_made.size(), 1u);
  EXPECT_EQ(requests_made[0].url, expected_patch_url);
  EXPECT_EQ(requests_made[0].method, "GET");
}

// Test GitHub compare URL fetching
TEST_F(PageContentFetcherTest, GithubCompareUrl) {
  GURL compare_url(
      "https://github.com/brave/din_djarin/compare/master...this-is-the-way");
  GURL expected_patch_url(
      "https://github.com/brave/din_djarin/compare/"
      "master...this-is-the-way.patch");

  NavigateAndCommit(compare_url);

  // Track the requests that are made
  std::vector<network::ResourceRequest> requests_made;
  test_url_loader_factory_->SetInterceptor(base::BindLambdaForTesting(
      [&requests_made](const network::ResourceRequest& request) {
        requests_made.push_back(request);
      }));

  // Set up network response for patch URL
  SimulateNetworkResponse(expected_patch_url, net::HTTP_OK, kGithubPatch);

  base::test::TestFuture<std::string, bool, std::string> future;
  fetcher_->FetchPageContent("", future.GetCallback());

  // Wait for the result
  auto [content, is_video, invalidation_token] = future.Get();

  EXPECT_EQ(content, kGithubPatch);
  EXPECT_FALSE(is_video);
  EXPECT_TRUE(invalidation_token.empty());

  // Verify the request was made to the patch URL
  EXPECT_EQ(requests_made.size(), 1u);
  EXPECT_EQ(requests_made[0].url, expected_patch_url);
  EXPECT_EQ(requests_made[0].method, "GET");
}

// Test GitHub blob URL fetching
TEST_F(PageContentFetcherTest, GithubBlobUrl) {
  GURL blob_url(
      "https://github.com/brave/din_djarin/blob/master/components/stardust/"
      "may/the/force/be/with/you.cc");
  GURL expected_raw_url(
      "https://github.com/brave/din_djarin/blob/master/components/stardust/"
      "may/the/force/be/with/you.cc?raw=true");

  NavigateAndCommit(blob_url);

  // Track the requests that are made
  std::vector<network::ResourceRequest> requests_made;
  test_url_loader_factory_->SetInterceptor(base::BindLambdaForTesting(
      [&requests_made](const network::ResourceRequest& request) {
        requests_made.push_back(request);
      }));

  // Set up network response for raw URL
  SimulateNetworkResponse(expected_raw_url, net::HTTP_OK, kGithubRawFile);

  base::test::TestFuture<std::string, bool, std::string> future;
  fetcher_->FetchPageContent("", future.GetCallback());

  // Wait for the result
  auto [content, is_video, invalidation_token] = future.Get();

  EXPECT_EQ(content, kGithubRawFile);
  EXPECT_FALSE(is_video);
  EXPECT_TRUE(invalidation_token.empty());

  // Verify the request was made to the raw URL
  EXPECT_EQ(requests_made.size(), 1u);
  EXPECT_EQ(requests_made[0].url, expected_raw_url);
  EXPECT_EQ(requests_made[0].method, "GET");
}

// Test GitHub commits URL fetching
TEST_F(PageContentFetcherTest, GithubCommitsUrl) {
  GURL commits_url("https://github.com/brave/din_djarin/commits/master");
  GURL expected_atom_url(
      "https://github.com/brave/din_djarin/commits/master.atom");

  NavigateAndCommit(commits_url);

  // Track the requests that are made
  std::vector<network::ResourceRequest> requests_made;
  test_url_loader_factory_->SetInterceptor(base::BindLambdaForTesting(
      [&requests_made](const network::ResourceRequest& request) {
        requests_made.push_back(request);
      }));

  // Set up network response for atom URL
  SimulateNetworkResponse(expected_atom_url, net::HTTP_OK, kGithubAtomFeed);

  base::test::TestFuture<std::string, bool, std::string> future;
  fetcher_->FetchPageContent("", future.GetCallback());

  // Wait for the result
  auto [content, is_video, invalidation_token] = future.Get();

  EXPECT_EQ(content, kGithubAtomFeed);
  EXPECT_FALSE(is_video);
  EXPECT_TRUE(invalidation_token.empty());

  // Verify the request was made to the atom URL
  EXPECT_EQ(requests_made.size(), 1u);
  EXPECT_EQ(requests_made[0].url, expected_atom_url);
  EXPECT_EQ(requests_made[0].method, "GET");
}

}  // namespace ai_chat
