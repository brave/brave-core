/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/speedreader/common/features.h"
#include "brave/components/speedreader/rust/ffi/speedreader.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace speedreader {

class SpeedreaderRewriterTestBase : public ::testing::Test {
 public:
  SpeedreaderRewriterTestBase() {
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir_);
    test_data_dir_ = test_data_dir_.AppendASCII("speedreader/rewriter");
    set_current_process_dir(test_data_dir_);
  }

  ~SpeedreaderRewriterTestBase() override = default;

  std::string GetFileContent(const std::string& filename) {
    std::string result;

    const auto full_path = current_process_dir_.AppendASCII(filename);
    /* void() for assert */
    [&]() { ASSERT_TRUE(base::ReadFileToString(full_path, &result)); }();
    return result;
  }

  std::string ProcessPage(const std::string& file_name,
                          const std::string& theme = {}) {
    auto rewriter = speedreader_.MakeRewriter(
        "https://test.com", RewriterType::RewriterReadability);
    rewriter->SetMinOutLength(100);
    rewriter->SetTheme(theme);
    const auto file_content = GetFileContent(file_name);
    rewriter->Write(file_content.data(), file_content.size());
    rewriter->End();
    return rewriter->GetOutput();
  }

  void CheckContent(const std::string& expected_content,
                    const std::string& filename) {
    EXPECT_EQ(GetFileContent(filename), expected_content) << expected_content;
  }

  const base::FilePath& test_data_dir() const { return test_data_dir_; }

  void set_current_process_dir(const base::FilePath& path) {
    current_process_dir_ = path;
  }

 private:
  SpeedReader speedreader_;
  base::FilePath test_data_dir_;
  base::FilePath current_process_dir_;
};

class SpeedreaderRewriterTest
    : public SpeedreaderRewriterTestBase,
      public ::testing::WithParamInterface<const char*> {};

INSTANTIATE_TEST_SUITE_P(,
                         SpeedreaderRewriterTest,
                         ::testing::Values("jsonld_shortest_desc",
                                           "meta_property_shortest_desc",
                                           "meta_name_shortest_desc",
                                           "no_span_root",
                                           "too_small_output"));

TEST_P(SpeedreaderRewriterTest, Check) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  const std::string input_file = std::string(GetParam()).append(".html");
  const std::string expected_file =
      std::string(GetParam()).append(".expected.html");

  const auto out = ProcessPage(input_file);
  CheckContent(out, expected_file);
}

class SpeedreaderRewriterThemeTest : public SpeedreaderRewriterTestBase {};

TEST_F(SpeedreaderRewriterThemeTest, SetTheme) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  const std::string input_file = "meta_name_shortest_desc.html";
  const std::string expected_file = "meta_name_shortest_desc.themed.html";
  const auto out = ProcessPage(input_file, "dark");
  CheckContent(out, expected_file);
}

class SpeedreaderRewriterPagesTest : public SpeedreaderRewriterTestBase {};

TEST_F(SpeedreaderRewriterPagesTest, GoodPages_News) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::FileEnumerator enumerator(
      test_data_dir().AppendASCII("pages/news_pages"), false,
      base::FileEnumerator::DIRECTORIES);

  base::FilePath domain;
  while (!(domain = enumerator.Next()).empty()) {
    SCOPED_TRACE(domain.BaseName());
    set_current_process_dir(domain);
    const auto out = ProcessPage("original.html");
    CheckContent(out, "distilled.html");
  }
}

TEST_F(SpeedreaderRewriterPagesTest, BadPages) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::FileEnumerator enumerator(
      test_data_dir().AppendASCII("pages/issues_pages"), false,
      base::FileEnumerator::DIRECTORIES);

  base::FilePath domain;
  while (!(domain = enumerator.Next()).empty()) {
    SCOPED_TRACE(domain.BaseName());
    set_current_process_dir(domain);
    const auto out = ProcessPage("original.html");

    if (GetFileContent("distilled.html") != out) {
      FAIL();
    }
  }
}

}  // namespace speedreader
