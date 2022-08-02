/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

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
  }

  ~SpeedreaderRewriterTestBase() override = default;

  std::string GetFileContent(const std::string& filename) {
    std::string result;

    const auto full_path = test_data_dir_.AppendASCII(filename);
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

 private:
  SpeedReader speedreader_;
  base::FilePath test_data_dir_;
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

}  // namespace speedreader
