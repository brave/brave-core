// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/text_file_extractor.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/strcat.h"
#include "base/test/test_future.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/strings/grit/blink_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace ai_chat {

class TextFileExtractorTest : public content::RenderViewHostTestHarness {
 public:
  TextFileExtractorTest()
      : content::RenderViewHostTestHarness(
            base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  std::unique_ptr<content::BrowserContext> CreateBrowserContext() override {
    return std::make_unique<TestingProfile>();
  }
};

// Without a real renderer producing text content, the extraction should
// time out and return nullopt.
TEST_F(TextFileExtractorTest, BytesOverload_TimeoutReturnsNullopt) {
  auto extractor = std::make_unique<TextFileExtractor>();
  base::test::TestFuture<std::optional<std::string>> future;

  std::vector<uint8_t> text_bytes = {'h', 'e', 'l', 'l', 'o'};
  extractor->ExtractText(browser_context(), std::move(text_bytes),
                         FILE_PATH_LITERAL("txt"), future.GetCallback());

  task_environment()->FastForwardBy(base::Seconds(31));

  auto result = future.Take();
  EXPECT_FALSE(result.has_value());
}

// Same timeout test but using the file-path overload (no temp file).
TEST_F(TextFileExtractorTest, PathOverload_TimeoutReturnsNullopt) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath txt_path = temp_dir.GetPath().AppendASCII("test.txt");
  ASSERT_TRUE(base::WriteFile(txt_path, "hello world"));

  auto extractor = std::make_unique<TextFileExtractor>();
  base::test::TestFuture<std::optional<std::string>> future;

  extractor->ExtractText(browser_context(), txt_path, future.GetCallback());

  task_environment()->FastForwardBy(base::Seconds(31));

  auto result = future.Take();
  EXPECT_FALSE(result.has_value());
}

// Destroying the extractor while an extraction is in-flight must not crash
// or leak.
TEST_F(TextFileExtractorTest, DestroyDuringExtraction_NoCrash) {
  auto extractor = std::make_unique<TextFileExtractor>();

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath txt_path = temp_dir.GetPath().AppendASCII("test.txt");
  ASSERT_TRUE(base::WriteFile(txt_path, "hello world"));

  bool callback_called = false;
  extractor->ExtractText(
      browser_context(), txt_path,
      base::BindOnce(
          [](bool* called, std::optional<std::string>) { *called = true; },
          &callback_called));

  // Destroy while extraction is in progress — should not crash.
  extractor.reset();

  EXPECT_FALSE(callback_called);
}

// The bytes overload writes to a temp file. Verify cleanup completes
// without crashing after timeout.
TEST_F(TextFileExtractorTest, BytesOverload_CleanupAfterTimeout) {
  auto extractor = std::make_unique<TextFileExtractor>();
  base::test::TestFuture<std::optional<std::string>> future;

  std::vector<uint8_t> text_bytes = {'h', 'e', 'l', 'l', 'o'};
  extractor->ExtractText(browser_context(), std::move(text_bytes),
                         FILE_PATH_LITERAL("txt"), future.GetCallback());

  task_environment()->FastForwardBy(base::Seconds(31));

  ASSERT_TRUE(future.Wait());
}

// The file-path overload should NOT delete the original file after extraction.
TEST_F(TextFileExtractorTest, PathOverload_OriginalFileNotDeleted) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath txt_path = temp_dir.GetPath().AppendASCII("keep_me.txt");
  ASSERT_TRUE(base::WriteFile(txt_path, "this file should persist"));

  auto extractor = std::make_unique<TextFileExtractor>();
  base::test::TestFuture<std::optional<std::string>> future;

  extractor->ExtractText(browser_context(), txt_path, future.GetCallback());

  task_environment()->FastForwardBy(base::Seconds(31));

  ASSERT_TRUE(future.Wait());

  // The original file must still exist — only temp files are cleaned up.
  EXPECT_TRUE(base::PathExists(txt_path));
}

// Unit tests for OnTextExtracted view-source stripping logic.
TEST_F(TextFileExtractorTest, OnTextExtracted_StripsViewSourcePrefix) {
  auto extractor = std::make_unique<TextFileExtractor>();
  base::test::TestFuture<std::optional<std::string>> future;
  extractor->callback_ = future.GetCallback();

  // Simulate view-source innerText: "<Line wrap label>\n\tline1\n\tline2"
  std::string line_wrap = l10n_util::GetStringUTF8(IDS_VIEW_SOURCE_LINE_WRAP);
  extractor->OnTextExtracted(
      base::Value(base::StrCat({line_wrap, "\n\tHello world\n\tSecond line"})));

  auto result = future.Take();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "Hello world\nSecond line");
}

TEST_F(TextFileExtractorTest, OnTextExtracted_StripsLeadingTabs) {
  auto extractor = std::make_unique<TextFileExtractor>();
  base::test::TestFuture<std::optional<std::string>> future;
  extractor->callback_ = future.GetCallback();

  // Tabs embedded within content (not at line start) should be preserved.
  std::string line_wrap = l10n_util::GetStringUTF8(IDS_VIEW_SOURCE_LINE_WRAP);
  extractor->OnTextExtracted(
      base::Value(base::StrCat({line_wrap, "\n\tcol1\tcol2\n\tcol3\tcol4"})));

  auto result = future.Take();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "col1\tcol2\ncol3\tcol4");
}

TEST_F(TextFileExtractorTest, OnTextExtracted_EmptyFile) {
  auto extractor = std::make_unique<TextFileExtractor>();
  base::test::TestFuture<std::optional<std::string>> future;
  extractor->callback_ = future.GetCallback();

  // Empty file in view-source returns just the label with no newline.
  std::string line_wrap = l10n_util::GetStringUTF8(IDS_VIEW_SOURCE_LINE_WRAP);
  extractor->OnTextExtracted(base::Value(line_wrap));

  auto result = future.Take();
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->empty());
}

TEST_F(TextFileExtractorTest, OnTextExtracted_NonStringReturnsNullopt) {
  auto extractor = std::make_unique<TextFileExtractor>();
  base::test::TestFuture<std::optional<std::string>> future;
  extractor->callback_ = future.GetCallback();

  extractor->OnTextExtracted(base::Value(42));

  auto result = future.Take();
  EXPECT_FALSE(result.has_value());
}

}  // namespace ai_chat
