/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <sys/xattr.h>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "brave/components/brave_referrals/browser/file_extended_attribute.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
constexpr char kXattrName[] = "com.brave.refcode";
constexpr char kXattrValue[] = "0xDEADFACE";
}  // namespace

class BraveFileExtendedAttributeTest : public testing::Test {
 public:
  BraveFileExtendedAttributeTest() = default;
  ~BraveFileExtendedAttributeTest() override = default;

  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    ASSERT_TRUE(
        base::CreateTemporaryFileInDir(temp_dir_.GetPath(), &test_file_path_));
    ASSERT_EQ(0,
              setxattr(test_file_path_.value().c_str(), kXattrName, kXattrValue,
                       strlen(kXattrValue), /*position=*/0, /*options=*/0));
  }

  void TearDown() override { base::DeletePathRecursively(temp_dir_.GetPath()); }

  const base::FilePath& test_file_path() { return test_file_path_; }

 private:
  base::ScopedTempDir temp_dir_;
  base::FilePath test_file_path_;
};

TEST_F(BraveFileExtendedAttributeTest, GetPromoCodeAttribute) {
  std::vector<char> value;
  // Test file has this extended attribute
  int result_errno =
      brave::GetFileExtendedAttribute(test_file_path(), kXattrName, &value);
  EXPECT_EQ(0, result_errno);
  std::string xattr_value(value.begin(), value.end());
  EXPECT_EQ(kXattrValue, xattr_value);
}

TEST_F(BraveFileExtendedAttributeTest, GetNonexistentAttribute) {
  std::vector<char> value;
  // Test file does NOT have this extended attribute
  int result_errno = brave::GetFileExtendedAttribute(
      test_file_path(), "com.brave.MadSheep", &value);
  EXPECT_EQ(ENOATTR, result_errno);
}
