// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/core/filename_sanitizer.h"

#include <string>

#include "base/strings/string_util.h"
#include "brave/components/brave_transfer/core/test/test_vectors.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_transfer {

TEST(BraveTransferFilenameSanitizerTest, Vectors) {
  base::DictValue vectors = test::LoadVectors("filename_sanitizer.json");
  const base::ListValue* cases = vectors.FindList("cases");
  ASSERT_TRUE(cases);
  ASSERT_FALSE(cases->empty());
  for (const base::Value& entry : *cases) {
    const base::DictValue& v = entry.GetDict();
    const std::string& input = *v.FindString("input");
    SCOPED_TRACE(test::HexString(std::string_view(input)));
    const std::string output = SanitizeFilename(input);
    EXPECT_EQ(output, *v.FindString("output"));
    EXPECT_FALSE(output.empty());
    EXPECT_LE(output.size(), 255u);
    EXPECT_TRUE(base::IsStringUTF8(output));
    EXPECT_EQ(SanitizeFilename(output), output);
  }
}

TEST(BraveTransferFilenameSanitizerTest, MalformedUtf8) {
  const std::string output = SanitizeFilename(
      "a\xff"
      "b.txt");
  EXPECT_EQ(output,
            "a\xEF\xBF\xBD"
            "b.txt");
  EXPECT_EQ(SanitizeFilename("\xc3"), "\xEF\xBF\xBD");
}

TEST(BraveTransferFilenameSanitizerTest, NeverAPath) {
  for (const char* name : {"../../etc/passwd", "..", ".", "/", "\\\\server\\x",
                           "C:\\Windows\\win.ini", "", "   "}) {
    SCOPED_TRACE(name);
    const std::string output = SanitizeFilename(name);
    EXPECT_FALSE(output.empty());
    EXPECT_EQ(output.find('/'), std::string::npos);
    EXPECT_EQ(output.find('\\'), std::string::npos);
    EXPECT_NE(output, ".");
    EXPECT_NE(output, "..");
  }
}

}  // namespace brave_transfer
