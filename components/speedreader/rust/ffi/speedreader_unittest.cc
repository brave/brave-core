/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/rust/ffi/speedreader.h"

#include <memory>
#include <cstring>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr char test_config[] = R"(
[
    {
        "domain": "example.com",
        "url_rules": [
            "||example.com/*/article/"
        ],
        "declarative_rewrite": {
            "main_content": [
                ".article-title",
                ".article-body"
            ],
            "main_content_cleanup": [
                ".hidden"
            ],
            "delazify": true,
            "fix_embeds": false,
            "content_script": null,
            "preprocess": []
        }
    },
    {
        "domain": "anotherexample.com",
        "url_rules": [
            "||anotherexample.com/article/"
        ]
    }
]
)";

}  // namespace

namespace speedreader {

TEST(SpeedreaderFFITest, URLReadable) {
  SpeedReader sr;
  ASSERT_TRUE(sr.deserialize(test_config, strlen(test_config)));
  std::string url_str = "https://example.com/news/article/topic/index.html";
  EXPECT_TRUE(sr.ReadableURL(url_str));
}

TEST(SpeedreaderFFITest, URLNotReadable) {
  SpeedReader sr;
  ASSERT_TRUE(sr.deserialize(test_config, strlen(test_config)));
  std::string url_str = "https://unknown.com/news/article/topic/index.html";
  EXPECT_FALSE(sr.ReadableURL(url_str));
}

TEST(SpeedreaderFFITest, URLInvalid) {
  SpeedReader sr;
  std::string url_str = "brave://about";
  EXPECT_FALSE(sr.ReadableURL(url_str));
}

TEST(SpeedreaderFFITest, URLEmpty) {
  SpeedReader sr;
  std::string url_str = "";
  EXPECT_FALSE(sr.ReadableURL(url_str));
}

TEST(SpeedreaderFFITest, FindRewriterType) {
  SpeedReader sr;
  ASSERT_TRUE(sr.deserialize(test_config, strlen(test_config)));
  std::string url_str = "https://example.com/news/article/topic/index.html";
  EXPECT_EQ(sr.RewriterTypeForURL(url_str), RewriterType::RewriterStreaming);
}

TEST(SpeedreaderFFITest, HeuristicsRewriterType) {
  SpeedReader sr;
  ASSERT_TRUE(sr.deserialize(test_config, strlen(test_config)));
  std::string url_str = "https://anotherexample.com/news/article/topic/index.html";
  EXPECT_EQ(sr.RewriterTypeForURL(url_str), RewriterType::RewriterHeuristics);
}

TEST(SpeedreaderFFITest, UnknownRewriterType) {
  SpeedReader sr;
  ASSERT_TRUE(sr.deserialize(test_config, strlen(test_config)));
  std::string url_str = "https://unknown.com/news/article/topic/index.html";
  EXPECT_EQ(sr.RewriterTypeForURL(url_str), RewriterType::RewriterUnknown);
}

TEST(SpeedreaderFFITest, RewriterCallback) {
  SpeedReader sr;
  ASSERT_TRUE(sr.deserialize(test_config, strlen(test_config)));
  std::string url_str = "https://example.com/news/article/topic/index.html";
  std::string output;
  auto callback = [](const char* chunk, size_t chunk_len, void* user_data) {
    std::string* out = static_cast<std::string*>(user_data);
    out->append(chunk, chunk_len);
  };
  auto rewriter =
      sr.RewriterNew(url_str, RewriterType::RewriterUnknown, callback, &output);
  const char* content1 = "<html><div class=\"article-body\">";
  ASSERT_EQ(rewriter->Write(content1, strlen(content1)), 0);
  const char* content2 = "hello world</div></html>";
  ASSERT_EQ(rewriter->Write(content2, strlen(content2)), 0);
  ASSERT_EQ(rewriter->End(), 0);
  EXPECT_EQ(output, "<html><div class=\"article-body\">hello world</div></html>");
  EXPECT_EQ(*rewriter->GetOutput(), "");
}

TEST(SpeedreaderFFITest, RewriterBufering) {
  SpeedReader sr;
  ASSERT_TRUE(sr.deserialize(test_config, strlen(test_config)));
  std::string url_str = "https://example.com/news/article/topic/index.html";
  auto rewriter = sr.RewriterNew(url_str);
  const char* content1 = "<html><div class=\"article-body\">";
  ASSERT_EQ(rewriter->Write(content1, strlen(content1)), 0);
  const char* content2 = "hello world</div></html>";
  ASSERT_EQ(rewriter->Write(content2, strlen(content2)), 0);
  ASSERT_EQ(rewriter->End(), 0);
  EXPECT_EQ(*rewriter->GetOutput(),
            "<html><div class=\"article-body\">hello world</div></html>");
}

TEST(SpeedreaderFFITest, RewriterBadSequence) {
  SpeedReader sr;
  ASSERT_TRUE(sr.deserialize(test_config, strlen(test_config)));
  std::string url_str = "https://example.com/news/article/topic/index.html";
  auto rewriter = sr.RewriterNew(url_str, RewriterType::RewriterUnknown);
  ASSERT_EQ(rewriter->End(), 0);
  const char* content = "hello";
  ASSERT_NE(rewriter->Write(content, strlen(content)), 0);
}

TEST(SpeedreaderFFITest, RewriterDoubleEnd) {
  SpeedReader sr;
  ASSERT_TRUE(sr.deserialize(test_config, strlen(test_config)));
  std::string url_str = "https://example.com/news/article/topic/index.html";
  auto rewriter = sr.RewriterNew(url_str, RewriterType::RewriterUnknown);
  ASSERT_EQ(rewriter->End(), 0);
  ASSERT_NE(rewriter->End(), 0);
}

TEST(SpeedreaderFFITest, RewriterParsingAmbiguity) {
  const char* ambiguity =
      "<select><div><style><div></div></style></div></select>";
  SpeedReader sr;
  ASSERT_TRUE(sr.deserialize(test_config, strlen(test_config)));
  std::string url_str = "https://example.com/news/article/topic/index.html";
  auto rewriter = sr.RewriterNew(url_str, RewriterType::RewriterUnknown);
  int write_ret = rewriter->Write(ambiguity, strlen(ambiguity));
  if (write_ret == 0) {
    int end_ret = rewriter->End();
    EXPECT_NE(end_ret, 0);
    EXPECT_NE(SpeedReader::TakeLastError(), "");
  } else {
    EXPECT_NE(SpeedReader::TakeLastError(), "");
  }
}

}  // namespace speedreader
