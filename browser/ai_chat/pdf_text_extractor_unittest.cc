// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/pdf_text_extractor.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/test/run_until.h"
#include "base/test/test_future.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class TestPdfTextExtractor : public PdfTextExtractor {
 public:
  content::WebContents* GetWebContentsForTesting() {
    return GetWebContents();
  }
};

class PdfTextExtractorTest : public content::RenderViewHostTestHarness {
 public:
  PdfTextExtractorTest()
      : content::RenderViewHostTestHarness(
            base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  std::unique_ptr<content::BrowserContext> CreateBrowserContext() override {
    return std::make_unique<TestingProfile>();
  }
};

// Without a real PDF viewer extension, the hidden WebContents will never
// produce a PDFDocumentHelper. The extraction should time out and return
// nullopt.
TEST_F(PdfTextExtractorTest, TimeoutReturnsNullopt) {
  auto extractor = std::make_unique<PdfTextExtractor>();
  base::test::TestFuture<std::optional<std::string>> future;

  std::vector<uint8_t> dummy_pdf = {0x25, 0x50, 0x44, 0x46};  // %PDF
  extractor->ExtractText(browser_context(), std::move(dummy_pdf),
                         FILE_PATH_LITERAL("pdf"), future.GetCallback());

  // Fast-forward past the 30s extraction timeout.
  // This also processes pending ThreadPool tasks (temp-file write).
  task_environment()->FastForwardBy(base::Seconds(31));

  auto result = future.Take();
  EXPECT_FALSE(result.has_value());
}

// Destroying after the hidden WebContents starts loading must not invoke the
// extraction callback.
TEST_F(PdfTextExtractorTest, DestroyAfterLoadStarts_DoesNotRunCallback) {
  auto extractor = std::make_unique<TestPdfTextExtractor>();
  base::test::TestFuture<std::optional<std::string>> future;

  std::vector<uint8_t> dummy_pdf = {0x25, 0x50, 0x44, 0x46};
  extractor->ExtractText(browser_context(), std::move(dummy_pdf),
                         FILE_PATH_LITERAL("pdf"), future.GetCallback());

  ASSERT_TRUE(base::test::RunUntil([&] {
    return extractor->GetWebContentsForTesting() != nullptr;
  }));
  extractor.reset();

  EXPECT_FALSE(future.IsReady());
}

// Verify the extraction and cleanup complete without crashing after timeout.
TEST_F(PdfTextExtractorTest, CleanupAfterTimeout) {
  auto extractor = std::make_unique<PdfTextExtractor>();
  base::test::TestFuture<std::optional<std::string>> future;

  std::vector<uint8_t> dummy_pdf = {0x25, 0x50, 0x44, 0x46};
  extractor->ExtractText(browser_context(), std::move(dummy_pdf),
                         FILE_PATH_LITERAL("pdf"), future.GetCallback());

  // Fast-forward past timeout to trigger cleanup.
  // This also processes pending ThreadPool tasks (temp-file write).
  task_environment()->FastForwardBy(base::Seconds(31));

  ASSERT_TRUE(future.Wait());
}

}  // namespace ai_chat
