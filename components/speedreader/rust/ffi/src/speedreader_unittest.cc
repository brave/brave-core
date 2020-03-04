/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "testing/gtest/include/gtest/gtest.h"
#include "brave/components/speedreader/rust/ffi/include/speedreader.hpp"
#include "base/files/file_path.h"
#include "base/files/file_util.h"

#include <memory>

namespace {

std::string LoadFile(const std::string& domain, const std::string& sample) {
  auto path =
      base::FilePath(FILE_PATH_LITERAL("brave"))
          .Append(FILE_PATH_LITERAL("components"))
          .Append(FILE_PATH_LITERAL("speedreader"))
          .Append(FILE_PATH_LITERAL("rust"))
          .Append(FILE_PATH_LITERAL("src"))
          .Append(FILE_PATH_LITERAL("data"))
          .Append(FILE_PATH_LITERAL("lolhtml"))
          .Append(FILE_PATH_LITERAL(domain))
          .Append(FILE_PATH_LITERAL(sample))
          .Append(FILE_PATH_LITERAL("init.html"));

  std::string value;
  const bool ok = ReadFileToString(path, &value);
  if (!ok) return {};
  return value;
}

}  // namespace

namespace speedreader {

TEST(SpeedreaderFFITest, URLReadable) {
  SpeedReader sr;
  std::string url_str = "https://cnn.com/news/article/topic/index.html";
  EXPECT_TRUE(sr.ReadableURL(url_str));
}

TEST(SpeedreaderFFITest, URLNotReadable) {
  SpeedReader sr;
  std::string url_str = "https://example.com/news/article/topic/index.html";
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
  std::string url_str = "https://cnn.com/news/article/topic/index.html";
  EXPECT_EQ(sr.RewriterTypeForURL(url_str), RewriterType::RewriterStreaming);
}

TEST(SpeedreaderFFITest, UnknownRewriterType) {
  SpeedReader sr;
  std::string url_str = "https://example.com/news/article/topic/index.html";
  EXPECT_EQ(sr.RewriterTypeForURL(url_str), RewriterType::RewriterUnknown);
}

TEST(SpeedreaderFFITest, RewriterCallback) {
  SpeedReader sr;
  std::string url_str = "https://cnn.com/news/article/topic/index.html";
  std::string output;
  auto callback = [](const char* chunk, size_t chunk_len, void* user_data) {
    std::string* out = static_cast<std::string*>(user_data);
    out->append(chunk, chunk_len);
  };
  auto rewriter = sr.RewriterNew(url_str, RewriterType::RewriterUnknown, callback, &output);
  const char* content1 = "<html><div class=\"pg-headline\">";
  ASSERT_EQ(rewriter->Write(content1, strlen(content1)), 0);
  const char* content2 = "hello world</div></html>";
  ASSERT_EQ(rewriter->Write(content2, strlen(content2)), 0);
  ASSERT_EQ(rewriter->End(), 0);
  EXPECT_EQ(output, "<div class=\"pg-headline\">hello world</div>");
  EXPECT_EQ(*rewriter->GetOutput(), "");
}

TEST(SpeedreaderFFITest, RewriterBufering) {
  SpeedReader sr;
  std::string url_str = "https://cnn.com/news/article/topic/index.html";
  auto rewriter = sr.RewriterNew(url_str);
  const char* content1 = "<html><div class=\"pg-headline\">";
  ASSERT_EQ(rewriter->Write(content1, strlen(content1)), 0);
  const char* content2 = "hello world</div></html>";
  ASSERT_EQ(rewriter->Write(content2, strlen(content2)), 0);
  ASSERT_EQ(rewriter->End(), 0);
  EXPECT_EQ(*rewriter->GetOutput(), "<div class=\"pg-headline\">hello world</div>");
}

TEST(SpeedreaderFFITest, RewriterHeuristicsBufering) {
  SpeedReader sr;
  std::string url_str = "http://url.com";
  auto rewriter = sr.RewriterNew(url_str, RewriterType::RewriterHeuristics);
  std::string content1 = LoadFile("edition.cnn.com", "2256488769395184997");
  ASSERT_EQ(rewriter->Write(content1.c_str(), content1.length()), 0);
  EXPECT_EQ(rewriter->End(), 0);
  EXPECT_NE(rewriter->GetOutput()->find("<article><header><span>"), std::string::npos);
}

TEST(SpeedreaderFFITest, RewriterBadSequence) {
  SpeedReader sr;
  std::string url_str = "https://cnn.com/news/article/topic/index.html";
  auto rewriter = sr.RewriterNew(url_str, RewriterType::RewriterUnknown);
  ASSERT_EQ(rewriter->End(), 0);
  const char* content = "hello";
  ASSERT_NE(rewriter->Write(content, strlen(content)), 0);
}

TEST(SpeedreaderFFITest, RewriterDoubleEnd) {
  SpeedReader sr;
  std::string url_str = "https://cnn.com/news/article/topic/index.html";
  auto rewriter = sr.RewriterNew(url_str, RewriterType::RewriterUnknown);
  ASSERT_EQ(rewriter->End(), 0);
  ASSERT_NE(rewriter->End(), 0);
}

void test_cpp_rewriter_parsing_ambiguity() {
  const char* ambiguity = "<select><div><style><div></div></style></div></select>";

  std::unique_ptr<SpeedReader> sr = std::make_unique<SpeedReader>();
  std::string url_str = "https://cnn.com/news/article/topic/index.html";
  auto rewriter = sr->RewriterNew(url_str, RewriterType::RewriterUnknown);
  int write_ret = rewriter->Write(ambiguity, strlen(ambiguity));
  int end_ret = 0;
  if (write_ret != 0) {
    end_ret = rewriter->End();
    ok(end_ret != 0);
    std::cout << "Error: " << SpeedReader::TakeLastError() << std::endl;
  } else {
    ok(write_ret != 0);
    std::cout << "Error: " << SpeedReader::TakeLastError() << std::endl;
  }
}

}  // namespace speedreader
