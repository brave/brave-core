/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_sponsored_source_util.h"

#include <vector>

#include "base/files/file_path.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_background_images {

namespace {

constexpr char kCreativeFilePath[] = "path/to/creative/index.html";

std::vector<Campaign> Campaigns() {
  std::vector<Campaign> campaigns;

  Campaign campaign;

  SponsoredBackground creative;
  creative.file_path = base::FilePath::FromASCII(kCreativeFilePath);
  campaign.backgrounds.push_back(creative);

  campaigns.push_back(campaign);
  return campaigns;
}

bool VerifyGetFilePathForRequestPathExpectation(
    const base::FilePath& expected_file_path,
    const base::FilePath& request_path) {
  // Normalize the file path to ensure that the comparison is consistent across
  // platforms.
  return expected_file_path.NormalizePathSeparators() ==
         MaybeGetFilePathForRequestPath(request_path, Campaigns());
}

}  // namespace

TEST(NTPSponsoredSourceUtilTest,
     GetFilePathWhenRequestPathIsReferencingCreativeDirectory) {
  EXPECT_TRUE(VerifyGetFilePathForRequestPathExpectation(
      base::FilePath(FILE_PATH_LITERAL("path/to/creative/styles.css")),
      /*request_path=*/base::FilePath(
          FILE_PATH_LITERAL("creative/styles.css"))));
  EXPECT_TRUE(VerifyGetFilePathForRequestPathExpectation(
      base::FilePath(FILE_PATH_LITERAL("path/to/creative/スタイル.css")),
      /*request_path=*/base::FilePath(
          FILE_PATH_LITERAL("creative/スタイル.css"))));
}

TEST(NTPSponsoredSourceUtilTest,
     GetFilePathWhenRequestPathIsReferencingChildCreativeDirectory) {
  EXPECT_TRUE(VerifyGetFilePathForRequestPathExpectation(
      base::FilePath(
          FILE_PATH_LITERAL("path/to/creative/subdirectory/image.png")),
      /*request_path=*/base::FilePath(
          FILE_PATH_LITERAL("creative/subdirectory/image.png"))));
  EXPECT_TRUE(VerifyGetFilePathForRequestPathExpectation(
      base::FilePath(FILE_PATH_LITERAL(
          "path/to/creative/multiple/subdirectories/image.png")),
      /*request_path=*/base::FilePath(
          FILE_PATH_LITERAL("creative/multiple/subdirectories/image.png"))));
}

TEST(NTPSponsoredSourceUtilTest,
     DoNotGetFilePathWhenRequestPathIsReferencingAnotherCreativeDirectory) {
  EXPECT_FALSE(MaybeGetFilePathForRequestPath(
      /*request_path=*/base::FilePath(
          FILE_PATH_LITERAL("path/to/another_creative/styles.css")),
      Campaigns()));
}

TEST(NTPSponsoredSourceUtilTest,
     DoNotGetFilePathWhenRequestPathIsReferencingCreativeDirectoryWithoutFile) {
  EXPECT_FALSE(MaybeGetFilePathForRequestPath(
      /*request_path=*/base::FilePath(FILE_PATH_LITERAL("path/to/creative/")),
      Campaigns()));
  EXPECT_FALSE(MaybeGetFilePathForRequestPath(
      /*request_path=*/base::FilePath(FILE_PATH_LITERAL("path/to/creative")),
      Campaigns()));
}

TEST(NTPSponsoredSourceUtilTest,
     DoNotGetFilePathWhenRequestPathIsReferencingParentDirectory) {
  EXPECT_FALSE(MaybeGetFilePathForRequestPath(
      /*request_path=*/base::FilePath(
          FILE_PATH_LITERAL("../path/to/creative/styles.css")),
      Campaigns()));
  EXPECT_FALSE(MaybeGetFilePathForRequestPath(
      /*request_path=*/base::FilePath(
          FILE_PATH_LITERAL("./path/to/creative/styles.css")),
      Campaigns()));
  EXPECT_FALSE(MaybeGetFilePathForRequestPath(
      /*request_path=*/base::FilePath(
          FILE_PATH_LITERAL(" /path/to/creative/styles.css")),
      Campaigns()));
  EXPECT_FALSE(MaybeGetFilePathForRequestPath(
      /*request_path=*/base::FilePath(
          FILE_PATH_LITERAL("\n/path/to/creative/styles.css")),
      Campaigns()));
  EXPECT_FALSE(MaybeGetFilePathForRequestPath(
      /*request_path=*/base::FilePath(
          FILE_PATH_LITERAL("\r/path/to/creative/styles.css")),
      Campaigns()));
  EXPECT_FALSE(MaybeGetFilePathForRequestPath(
      /*request_path=*/base::FilePath(
          FILE_PATH_LITERAL("\t/path/to/creative/styles.css")),
      Campaigns()));
}

TEST(NTPSponsoredSourceUtilTest, DoNotGetFilePathWhenRequestPathIsMalformed) {
  EXPECT_FALSE(MaybeGetFilePathForRequestPath(
      /*request_path=*/base::FilePath(
          FILE_PATH_LITERAL("::malformed_path_to_nowhere::")),
      Campaigns()));
}

TEST(NTPSponsoredSourceUtilTest, DoNotGetFilePathWhenRequestPathIsEmpty) {
  EXPECT_FALSE(MaybeGetFilePathForRequestPath(
      /*request_path=*/base::FilePath(FILE_PATH_LITERAL("")), Campaigns()));
}

}  // namespace ntp_background_images
