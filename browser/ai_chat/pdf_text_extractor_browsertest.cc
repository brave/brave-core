// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/pdf_text_extractor.h"

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

class PdfTextExtractorBrowserTest : public InProcessBrowserTest {
 protected:
  base::FilePath GetTestPdfPath(std::string_view filename) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    return base::PathService::CheckedGet(brave::DIR_TEST_DATA)
        .AppendASCII("leo")
        .AppendASCII(filename);
  }

  content::BrowserContext* browser_context() { return browser()->profile(); }
};

IN_PROC_BROWSER_TEST_F(PdfTextExtractorBrowserTest, PathOverload_ExtractsText) {
  base::FilePath pdf_path = GetTestPdfPath("dummy.pdf");

  auto extractor = std::make_unique<PdfTextExtractor>();
  base::test::TestFuture<std::optional<std::string>> future;

  extractor->ExtractText(browser_context(), pdf_path, future.GetCallback());

  auto result = future.Take();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "This is the way\nI have spoken");
}

IN_PROC_BROWSER_TEST_F(PdfTextExtractorBrowserTest,
                       BytesOverload_ExtractsText) {
  std::optional<std::vector<uint8_t>> pdf_bytes;
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    pdf_bytes = base::ReadFileToBytes(GetTestPdfPath("dummy.pdf"));
  }
  ASSERT_TRUE(pdf_bytes.has_value());

  auto extractor = std::make_unique<PdfTextExtractor>();
  base::test::TestFuture<std::optional<std::string>> future;

  extractor->ExtractText(browser_context(), std::move(*pdf_bytes),
                         future.GetCallback());

  auto result = future.Take();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "This is the way\nI have spoken");
}

IN_PROC_BROWSER_TEST_F(PdfTextExtractorBrowserTest, EmptyPdf_ReturnsEmpty) {
  base::FilePath pdf_path = GetTestPdfPath("empty_pdf.pdf");

  auto extractor = std::make_unique<PdfTextExtractor>();
  base::test::TestFuture<std::optional<std::string>> future;

  extractor->ExtractText(browser_context(), pdf_path, future.GetCallback());

  auto result = future.Take();
  // Empty PDF should either return empty string or nullopt.
  if (result.has_value()) {
    EXPECT_TRUE(result->empty());
  }
}

}  // namespace ai_chat
