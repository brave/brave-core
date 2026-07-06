/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_sponsored_sites_data.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/values_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ntp_background_images {

TEST(NTPSponsoredSitesDataTest, EmptyJson) {
  base::DictValue dict;
  base::FilePath installed_dir(FILE_PATH_LITERAL("ntp_sponsored_sites_data"));
  NTPSponsoredSitesData data(dict, installed_dir, /*url_prefix=*/"");
  EXPECT_FALSE(data.IsValid());
}

TEST(NTPSponsoredSitesDataTest, EmptyTiles) {
  base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 1,
        "tiles": [
        ]
      })");
  base::FilePath installed_dir(FILE_PATH_LITERAL("ntp_sponsored_sites_data"));
  NTPSponsoredSitesData data(dict, installed_dir, /*url_prefix=*/"");
  EXPECT_FALSE(data.IsValid());
}

TEST(NTPSponsoredSitesDataTest, MissingTilesKey) {
  base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 1
      })");
  base::FilePath installed_dir(FILE_PATH_LITERAL("ntp_sponsored_sites_data"));
  NTPSponsoredSitesData data(dict, installed_dir, /*url_prefix=*/"");
  EXPECT_FALSE(data.IsValid());
}

TEST(NTPSponsoredSitesDataTest, RejectsUnsupportedSchemaVersion) {
  base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 2,
        "tiles": [
        ]
      })");
  base::FilePath installed_dir(FILE_PATH_LITERAL("ntp_sponsored_sites_data"));
  NTPSponsoredSitesData data(dict, installed_dir, /*url_prefix=*/"");
  EXPECT_FALSE(data.IsValid());
}

TEST(NTPSponsoredSitesDataTest, ParseSponsoredSite) {
  base::ScopedTempDir installed_dir;
  ASSERT_TRUE(installed_dir.CreateUniqueTempDir());

  const base::FilePath image_dir = installed_dir.GetPath().AppendASCII("foo");
  ASSERT_TRUE(base::CreateDirectory(image_dir));
  ASSERT_TRUE(base::WriteFile(image_dir.AppendASCII("logo.png"), ""));

  base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 1,
        "tiles": [
          {
            "version": 1,
            "title": "foo",
            "adDisclosure": "Ad",
            "targetUrl": "https://foo.com",
            "image": {
              "relativeUrl": "foo/logo.png"
            }
          }
        ]
      })");
  NTPSponsoredSitesData data(
      dict, installed_dir.GetPath(),
      /*url_prefix=*/"chrome://branded-wallpaper/sponsored-images/");
  EXPECT_TRUE(data.IsValid());

  EXPECT_THAT(data.sites,
              testing::ElementsAre(testing::FieldsAre(
                  "chrome://branded-wallpaper/sponsored-images/foo/logo.png",
                  "foo", "Ad", GURL("https://foo.com"))));
}

TEST(NTPSponsoredSitesDataTest, RejectsSiteWithUnsupportedTileVersion) {
  base::ScopedTempDir installed_dir;
  ASSERT_TRUE(installed_dir.CreateUniqueTempDir());

  base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 1,
        "tiles": [
          {
            "version": 2,
            "title": "foo",
            "adDisclosure": "Ad",
            "targetUrl": "https://foo.com",
            "image": {
              "relativeUrl": "foo/logo.png"
            }
          }
        ]
      })");
  NTPSponsoredSitesData data(dict, installed_dir.GetPath(),
                             /*url_prefix=*/"");
  EXPECT_FALSE(data.IsValid());
}

TEST(NTPSponsoredSitesDataTest, RejectsSiteMissingTitle) {
  base::ScopedTempDir installed_dir;
  ASSERT_TRUE(installed_dir.CreateUniqueTempDir());

  base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 1,
        "tiles": [
          {
            "version": 1,
            "adDisclosure": "Ad",
            "targetUrl": "https://foo.com",
            "image": {
              "relativeUrl": "foo/logo.png"
            }
          }
        ]
      })");
  NTPSponsoredSitesData data(dict, installed_dir.GetPath(),
                             /*url_prefix=*/"");
  EXPECT_FALSE(data.IsValid());
}

TEST(NTPSponsoredSitesDataTest, KeepsSiteWithMissingAdDisclosure) {
  base::ScopedTempDir installed_dir;
  ASSERT_TRUE(installed_dir.CreateUniqueTempDir());

  const base::FilePath image_dir = installed_dir.GetPath().AppendASCII("foo");
  ASSERT_TRUE(base::CreateDirectory(image_dir));
  ASSERT_TRUE(base::WriteFile(image_dir.AppendASCII("logo.png"), ""));

  base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 1,
        "tiles": [
          {
            "version": 1,
            "title": "foo",
            "targetUrl": "https://foo.com",
            "image": {
              "relativeUrl": "foo/logo.png"
            }
          }
        ]
      })");
  NTPSponsoredSitesData data(dict, installed_dir.GetPath(),
                             /*url_prefix=*/"");
  EXPECT_TRUE(data.IsValid());

  EXPECT_THAT(data.sites, testing::ElementsAre(testing::Field(
                              &NTPSponsoredSite::ad_disclosure, "")));
}

TEST(NTPSponsoredSitesDataTest, RejectsSiteMissingTargetUrl) {
  base::ScopedTempDir installed_dir;
  ASSERT_TRUE(installed_dir.CreateUniqueTempDir());

  base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 1,
        "tiles": [
          {
            "version": 1,
            "title": "foo",
            "adDisclosure": "Ad",
            "image": {
              "relativeUrl": "foo/logo.png"
            }
          }
        ]
      })");
  NTPSponsoredSitesData data(dict, installed_dir.GetPath(),
                             /*url_prefix=*/"");
  EXPECT_FALSE(data.IsValid());
}

TEST(NTPSponsoredSitesDataTest, RejectsSiteMissingImageKey) {
  base::ScopedTempDir installed_dir;
  ASSERT_TRUE(installed_dir.CreateUniqueTempDir());

  base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 1,
        "tiles": [
          {
            "version": 1,
            "title": "foo",
            "adDisclosure": "Ad",
            "targetUrl": "https://foo.com"
          }
        ]
      })");
  NTPSponsoredSitesData data(dict, installed_dir.GetPath(),
                             /*url_prefix=*/"");
  EXPECT_FALSE(data.IsValid());
}

TEST(NTPSponsoredSitesDataTest, RejectsSiteMissingImageRelativeUrl) {
  base::ScopedTempDir installed_dir;
  ASSERT_TRUE(installed_dir.CreateUniqueTempDir());

  base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 1,
        "tiles": [
          {
            "version": 1,
            "title": "foo",
            "adDisclosure": "Ad",
            "targetUrl": "https://foo.com",
            "image": {
            }
          }
        ]
      })");
  NTPSponsoredSitesData data(dict, installed_dir.GetPath(),
                             /*url_prefix=*/"");
  EXPECT_FALSE(data.IsValid());
}

TEST(NTPSponsoredSitesDataTest, SkipsNonDictTileEntries) {
  base::ScopedTempDir installed_dir;
  ASSERT_TRUE(installed_dir.CreateUniqueTempDir());

  const base::FilePath image_dir = installed_dir.GetPath().AppendASCII("foo");
  ASSERT_TRUE(base::CreateDirectory(image_dir));
  ASSERT_TRUE(base::WriteFile(image_dir.AppendASCII("logo.png"), ""));

  base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 1,
        "tiles": [
          "not a dict",
          {
            "version": 1,
            "title": "foo",
            "adDisclosure": "Ad",
            "targetUrl": "https://foo.com",
            "image": {
              "relativeUrl": "foo/logo.png"
            }
          }
        ]
      })");
  NTPSponsoredSitesData data(dict, installed_dir.GetPath(),
                             /*url_prefix=*/"");
  EXPECT_TRUE(data.IsValid());

  EXPECT_THAT(data.sites, testing::ElementsAre(
                              testing::Field(&NTPSponsoredSite::title, "foo")));
}

TEST(NTPSponsoredSitesDataTest, RejectsSiteWithInvalidTargetUrl) {
  base::ScopedTempDir installed_dir;
  ASSERT_TRUE(installed_dir.CreateUniqueTempDir());

  base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 1,
        "tiles": [
          {
            "version": 1,
            "title": "foo",
            "adDisclosure": "Ad",
            "targetUrl": "invalid",
            "image": {
              "relativeUrl": "foo/logo.png"
            }
          }
        ]
      })");
  NTPSponsoredSitesData data(dict, installed_dir.GetPath(),
                             /*url_prefix=*/"");
  EXPECT_FALSE(data.IsValid());
}

TEST(NTPSponsoredSitesDataTest, RejectsSiteWithNonHttpTargetUrlScheme) {
  base::ScopedTempDir installed_dir;
  ASSERT_TRUE(installed_dir.CreateUniqueTempDir());

  base::DictValue dict = base::test::ParseJsonDict(R"JSON(
      {
        "schemaVersion": 1,
        "tiles": [
          {
            "version": 1,
            "title": "foo",
            "adDisclosure": "Ad",
            "targetUrl": "javascript:alert(1)",
            "image": {
              "relativeUrl": "foo/logo.png"
            }
          }
        ]
      })JSON");
  NTPSponsoredSitesData data(dict, installed_dir.GetPath(),
                             /*url_prefix=*/"");
  EXPECT_FALSE(data.IsValid());
}

TEST(NTPSponsoredSitesDataTest, RejectsSiteWhenImageFileIsMissing) {
  base::ScopedTempDir installed_dir;
  ASSERT_TRUE(installed_dir.CreateUniqueTempDir());

  base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 1,
        "tiles": [
          {
            "version": 1,
            "title": "foo",
            "adDisclosure": "Ad",
            "targetUrl": "https://foo.com",
            "image": {
              "relativeUrl": "foo/logo.png"
            }
          }
        ]
      })");
  NTPSponsoredSitesData data(dict, installed_dir.GetPath(),
                             /*url_prefix=*/"");
  EXPECT_FALSE(data.IsValid());
}

TEST(NTPSponsoredSitesDataTest, KeepsOnlyValidSitesAmongMultiple) {
  base::ScopedTempDir installed_dir;
  ASSERT_TRUE(installed_dir.CreateUniqueTempDir());

  // "bar"'s image file is created too, so it's dropped for its invalid
  // target URL specifically, not for a missing image.
  for (const char* site : {"foo", "bar", "baz"}) {
    const base::FilePath image_dir = installed_dir.GetPath().AppendASCII(site);
    ASSERT_TRUE(base::CreateDirectory(image_dir));
    ASSERT_TRUE(base::WriteFile(image_dir.AppendASCII("logo.png"), ""));
  }

  base::DictValue dict = base::test::ParseJsonDict(R"(
      {
        "schemaVersion": 1,
        "tiles": [
          {
            "version": 1,
            "title": "foo",
            "adDisclosure": "Ad",
            "targetUrl": "https://foo.com",
            "image": {
              "relativeUrl": "foo/logo.png"
            }
          },
          {
            "version": 1,
            "title": "bar",
            "adDisclosure": "Ad",
            "targetUrl": "invalid",
            "image": {
              "relativeUrl": "bar/logo.png"
            }
          },
          {
            "version": 1,
            "title": "baz",
            "adDisclosure": "Ad",
            "targetUrl": "https://baz.com",
            "image": {
              "relativeUrl": "baz/logo.png"
            }
          }
        ]
      })");
  NTPSponsoredSitesData data(dict, installed_dir.GetPath(),
                             /*url_prefix=*/"");
  EXPECT_TRUE(data.IsValid());

  EXPECT_THAT(data.sites, testing::ElementsAre(
                              testing::Field(&NTPSponsoredSite::title, "foo"),
                              testing::Field(&NTPSponsoredSite::title, "baz")));
}

}  // namespace ntp_background_images
