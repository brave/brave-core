/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_sponsored_sites_data.h"

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/test/values_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ntp_background_images {

namespace {

base::FilePath GetTestDataDir() {
  return base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
      .AppendASCII("brave")
      .AppendASCII("test")
      .AppendASCII("data")
      .AppendASCII("components")
      .AppendASCII("ntp_sponsored_sites_data");
}

}  // namespace

TEST(NTPSponsoredSitesDataTest, EmptyJson) {
  base::DictValue dict;
  NTPSponsoredSitesData data(dict, GetTestDataDir(), /*url_prefix=*/"");
  EXPECT_THAT(data.sites, testing::IsEmpty());
}

TEST(NTPSponsoredSitesDataTest, EmptyTiles) {
  const base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 1,
        "tiles": [
        ]
      })");
  NTPSponsoredSitesData data(dict, GetTestDataDir(), /*url_prefix=*/"");
  EXPECT_THAT(data.sites, testing::IsEmpty());
}

TEST(NTPSponsoredSitesDataTest, MissingTilesKey) {
  const base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 1
      })");
  NTPSponsoredSitesData data(dict, GetTestDataDir(), /*url_prefix=*/"");
  EXPECT_THAT(data.sites, testing::IsEmpty());
}

TEST(NTPSponsoredSitesDataTest, RejectsUnsupportedSchemaVersion) {
  const base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 0,
        "tiles": [
        ]
      })");
  NTPSponsoredSitesData data(dict, GetTestDataDir(), /*url_prefix=*/"");
  EXPECT_THAT(data.sites, testing::IsEmpty());
}

TEST(NTPSponsoredSitesDataTest, ParseSponsoredSite) {
  const base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 1,
        "tiles": [
          {
            "version": 1,
            "title": "foo",
            "adDisclosure": "Sponsored",
            "targetUrl": "https://foo.com",
            "image": {
              "relativeUrl": "tiles/foo.webp"
            }
          }
        ]
      })");
  NTPSponsoredSitesData data(
      dict, GetTestDataDir(),
      /*url_prefix=*/"chrome://branded-wallpaper/sponsored-images/");
  EXPECT_THAT(data.sites,
              testing::ElementsAre(testing::FieldsAre(
                  /*relative_image_url_spec=*/
                  "chrome://branded-wallpaper/sponsored-images/tiles/foo.webp",
                  /*title=*/"foo", /*ad_disclosure=*/"Sponsored",
                  /*target_url=*/GURL("https://foo.com"))));
}

TEST(NTPSponsoredSitesDataTest, RejectsSiteWithUnsupportedTileVersion) {
  const base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 1,
        "tiles": [
          {
            "version": 0,
            "title": "foo",
            "adDisclosure": "Sponsored",
            "targetUrl": "https://foo.com",
            "image": {
              "relativeUrl": "tiles/foo.webp"
            }
          }
        ]
      })");
  NTPSponsoredSitesData data(dict, GetTestDataDir(), /*url_prefix=*/"");
  EXPECT_THAT(data.sites, testing::IsEmpty());
}

TEST(NTPSponsoredSitesDataTest, RejectsSiteMissingTitle) {
  const base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 1,
        "tiles": [
          {
            "version": 1,
            "adDisclosure": "Sponsored",
            "targetUrl": "https://foo.com",
            "image": {
              "relativeUrl": "tiles/foo.webp"
            }
          }
        ]
      })");
  NTPSponsoredSitesData data(dict, GetTestDataDir(), /*url_prefix=*/"");
  EXPECT_THAT(data.sites, testing::IsEmpty());
}

TEST(NTPSponsoredSitesDataTest, KeepsSiteWithMissingAdDisclosure) {
  const base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 1,
        "tiles": [
          {
            "version": 1,
            "title": "foo",
            "targetUrl": "https://foo.com",
            "image": {
              "relativeUrl": "tiles/foo.webp"
            }
          }
        ]
      })");
  NTPSponsoredSitesData data(dict, GetTestDataDir(), /*url_prefix=*/"");
  EXPECT_THAT(data.sites, testing::ElementsAre(testing::Field(
                              &NTPSponsoredSite::ad_disclosure, "")));
}

TEST(NTPSponsoredSitesDataTest, RejectsSiteMissingTargetUrl) {
  const base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 1,
        "tiles": [
          {
            "version": 1,
            "title": "foo",
            "adDisclosure": "Sponsored",
            "image": {
              "relativeUrl": "tiles/foo.webp"
            }
          }
        ]
      })");
  NTPSponsoredSitesData data(dict, GetTestDataDir(), /*url_prefix=*/"");
  EXPECT_THAT(data.sites, testing::IsEmpty());
}

TEST(NTPSponsoredSitesDataTest, RejectsSiteMissingImageKey) {
  const base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 1,
        "tiles": [
          {
            "version": 1,
            "title": "foo",
            "adDisclosure": "Sponsored",
            "targetUrl": "https://foo.com"
          }
        ]
      })");
  NTPSponsoredSitesData data(dict, GetTestDataDir(), /*url_prefix=*/"");
  EXPECT_THAT(data.sites, testing::IsEmpty());
}

TEST(NTPSponsoredSitesDataTest, RejectsSiteMissingImageRelativeUrl) {
  const base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 1,
        "tiles": [
          {
            "version": 1,
            "title": "foo",
            "adDisclosure": "Sponsored",
            "targetUrl": "https://foo.com",
            "image": {
            }
          }
        ]
      })");
  NTPSponsoredSitesData data(dict, GetTestDataDir(), /*url_prefix=*/"");
  EXPECT_THAT(data.sites, testing::IsEmpty());
}

TEST(NTPSponsoredSitesDataTest, SkipsNonDictTileEntries) {
  const base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 1,
        "tiles": [
          "not a dict",
          {
            "version": 1,
            "title": "foo",
            "adDisclosure": "Sponsored",
            "targetUrl": "https://foo.com",
            "image": {
              "relativeUrl": "tiles/foo.webp"
            }
          }
        ]
      })");
  NTPSponsoredSitesData data(dict, GetTestDataDir(), /*url_prefix=*/"");
  EXPECT_THAT(data.sites, testing::ElementsAre(
                              testing::Field(&NTPSponsoredSite::title, "foo")));
}

TEST(NTPSponsoredSitesDataTest, RejectsSiteWithInvalidTargetUrl) {
  const base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 1,
        "tiles": [
          {
            "version": 1,
            "title": "foo",
            "adDisclosure": "Sponsored",
            "targetUrl": "invalid",
            "image": {
              "relativeUrl": "tiles/foo.webp"
            }
          }
        ]
      })");
  NTPSponsoredSitesData data(dict, GetTestDataDir(), /*url_prefix=*/"");
  EXPECT_THAT(data.sites, testing::IsEmpty());
}

TEST(NTPSponsoredSitesDataTest, RejectsSiteWithNonHttpTargetUrlScheme) {
  const base::DictValue dict = base::test::ParseJsonDict(R"JSON(
      {
        "schemaVersion": 1,
        "tiles": [
          {
            "version": 1,
            "title": "foo",
            "adDisclosure": "Sponsored",
            "targetUrl": "javascript:alert(1)",
            "image": {
              "relativeUrl": "tiles/foo.webp"
            }
          }
        ]
      })JSON");
  NTPSponsoredSitesData data(dict, GetTestDataDir(), /*url_prefix=*/"");
  EXPECT_THAT(data.sites, testing::IsEmpty());
}

TEST(NTPSponsoredSitesDataTest, RejectsSiteWhenImageFileIsMissing) {
  const base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 1,
        "tiles": [
          {
            "version": 1,
            "title": "foo",
            "adDisclosure": "Sponsored",
            "targetUrl": "https://foo.com",
            "image": {
              "relativeUrl": "tiles/missing.webp"
            }
          }
        ]
      })");
  NTPSponsoredSitesData data(dict, GetTestDataDir(), /*url_prefix=*/"");
  EXPECT_THAT(data.sites, testing::IsEmpty());
}

TEST(NTPSponsoredSitesDataTest, KeepsOnlyValidSitesAmongMultiple) {
  const base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 1,
        "tiles": [
          {
            "version": 1,
            "title": "foo",
            "adDisclosure": "foo",
            "targetUrl": "https://foo.com",
            "image": {
              "relativeUrl": "tiles/foo.webp"
            }
          },
          {
            "version": 1,
            "title": "bar",
            "adDisclosure": "bar",
            "targetUrl": "invalid",
            "image": {
              "relativeUrl": "tiles/bar.webp"
            }
          },
          {
            "version": 1,
            "title": "baz",
            "adDisclosure": "baz",
            "targetUrl": "https://baz.com",
            "image": {
              "relativeUrl": "tiles/baz.webp"
            }
          }
        ]
      })");
  NTPSponsoredSitesData data(dict, GetTestDataDir(), /*url_prefix=*/"");
  EXPECT_THAT(
      data.sites,
      testing::ElementsAre(
          testing::FieldsAre(
              /*relative_image_url_spec=*/"tiles/foo.webp", /*title=*/"foo",
              /*ad_disclosure=*/"foo", /*target_url=*/GURL("https://foo.com")),
          testing::FieldsAre(
              /*relative_image_url_spec=*/"tiles/baz.webp", /*title=*/"baz",
              /*ad_disclosure=*/"baz",
              /*target_url=*/GURL("https://baz.com"))));
}

}  // namespace ntp_background_images
