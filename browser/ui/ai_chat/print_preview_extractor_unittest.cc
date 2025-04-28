// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/ai_chat/print_preview_extractor.h"

#include <memory>

#include "base/test/test_future.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "printing/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_PRINT_PREVIEW));

namespace ai_chat {

class PrintPreviewExtractorTest : public ChromeRenderViewHostTestHarness {
 public:
  PrintPreviewExtractorTest() = default;
  ~PrintPreviewExtractorTest() override = default;

  PrintPreviewExtractorTest(const PrintPreviewExtractorTest&) = delete;
  PrintPreviewExtractorTest& operator=(const PrintPreviewExtractorTest&) =
      delete;

 protected:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    NavigateAndCommit(GURL("https://brave.com/"),
                      ui::PageTransition::PAGE_TRANSITION_FIRST);
    extractor_ = std::make_unique<PrintPreviewExtractor>(web_contents());
  }
  void TearDown() override {
    extractor_.reset();
    ChromeRenderViewHostTestHarness::TearDown();
  }

 protected:
  std::unique_ptr<PrintPreviewExtractor> extractor_;
};

TEST_F(PrintPreviewExtractorTest, CapturePdfWithNotPdf) {
  ASSERT_NE(web_contents()->GetContentsMimeType(), "application/pdf");
  base::test::TestFuture<
      base::expected<std::vector<std::vector<uint8_t>>, std::string>>
      future;
  extractor_->CapturePdf(future.GetCallback());
  auto result = future.Take();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Not pdf content");
}

}  // namespace ai_chat
