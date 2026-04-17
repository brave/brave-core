// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/text_file_extractor.h"

#include <optional>
#include <string>
#include <vector>

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/test/test_future.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

// Expected content of test/data/leo/dummy.txt
constexpr char kExpectedTextContent[] =
    "Hello from a text file.\nThis is line two.";

class TextFileExtractorBrowserTest : public InProcessBrowserTest {
 protected:
  base::FilePath GetTestFilePath(std::string_view filename) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    return base::PathService::CheckedGet(brave::DIR_TEST_DATA)
        .AppendASCII("leo")
        .AppendASCII(filename);
  }

  content::BrowserContext* browser_context() { return browser()->profile(); }
};

IN_PROC_BROWSER_TEST_F(TextFileExtractorBrowserTest,
                       PathOverload_ExtractsText) {
  base::FilePath txt_path = GetTestFilePath("dummy.txt");

  auto extractor = std::make_unique<TextFileExtractor>();
  base::test::TestFuture<std::optional<std::string>> future;

  extractor->ExtractText(browser_context(), txt_path, future.GetCallback());

  auto result = future.Take();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, kExpectedTextContent);
}

IN_PROC_BROWSER_TEST_F(TextFileExtractorBrowserTest,
                       BytesOverload_ExtractsText) {
  std::optional<std::vector<uint8_t>> file_bytes;
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    file_bytes = base::ReadFileToBytes(GetTestFilePath("dummy.txt"));
  }
  ASSERT_TRUE(file_bytes.has_value());

  auto extractor = std::make_unique<TextFileExtractor>();
  base::test::TestFuture<std::optional<std::string>> future;

  extractor->ExtractText(browser_context(), std::move(*file_bytes),
                         FILE_PATH_LITERAL("txt"), future.GetCallback());

  auto result = future.Take();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, kExpectedTextContent);
}

// HTML files must not be rendered — the extracted text should be the raw
// source containing HTML tags, not the rendered text content.
IN_PROC_BROWSER_TEST_F(TextFileExtractorBrowserTest,
                       PathOverload_HtmlNotRendered) {
  base::FilePath html_path = GetTestFilePath("dummy.html");

  auto extractor = std::make_unique<TextFileExtractor>();
  base::test::TestFuture<std::optional<std::string>> future;

  extractor->ExtractText(browser_context(), html_path, future.GetCallback());

  auto result = future.Take();
  ASSERT_TRUE(result.has_value());
  // Raw source must contain HTML tags — proves it was NOT rendered.
  EXPECT_NE(result->find("<p>Hello from an HTML file.</p>"), std::string::npos);
  EXPECT_NE(result->find("<script>"), std::string::npos);
}

IN_PROC_BROWSER_TEST_F(TextFileExtractorBrowserTest,
                       BytesOverload_HtmlNotRendered) {
  std::optional<std::vector<uint8_t>> file_bytes;
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    file_bytes = base::ReadFileToBytes(GetTestFilePath("dummy.html"));
  }
  ASSERT_TRUE(file_bytes.has_value());

  auto extractor = std::make_unique<TextFileExtractor>();
  base::test::TestFuture<std::optional<std::string>> future;

  extractor->ExtractText(browser_context(), std::move(*file_bytes),
                         FILE_PATH_LITERAL("html"), future.GetCallback());

  auto result = future.Take();
  ASSERT_TRUE(result.has_value());
  EXPECT_NE(result->find("<p>Hello from an HTML file.</p>"), std::string::npos);
  EXPECT_NE(result->find("<script>"), std::string::npos);
}

IN_PROC_BROWSER_TEST_F(TextFileExtractorBrowserTest, EmptyFile_ReturnsEmpty) {
  // Create a temporary empty file
  base::FilePath temp_path;
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(base::CreateTemporaryFile(&temp_path));
  }

  auto extractor = std::make_unique<TextFileExtractor>();
  base::test::TestFuture<std::optional<std::string>> future;

  extractor->ExtractText(browser_context(), temp_path, future.GetCallback());

  auto result = future.Take();
  // Empty file should either return nullopt or empty string.
  if (result.has_value()) {
    EXPECT_TRUE(result->empty());
  }

  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    base::DeleteFile(temp_path);
  }
}

}  // namespace ai_chat
