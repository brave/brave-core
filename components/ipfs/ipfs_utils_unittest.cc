/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_utils.h"

#include <string>
#include <vector>

#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "net/base/url_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using IpfsUtilsUnitTest = testing::Test;

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURINotIPFSScheme) {
  GURL url(
      base::StrCat({ipfs::GetDefaultIPFSGateway().spec(),
                    "/ipfs/QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG"}));
  GURL new_url;
  ASSERT_FALSE(ipfs::TranslateIPFSURI(url, &new_url, false));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSScheme) {
  GURL url("ipfs://QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, false));
  EXPECT_EQ(new_url,
            GURL(base::StrCat(
                {ipfs::GetDefaultIPFSGateway().spec(),
                 "ipfs/QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG"})));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPNSScheme) {
  GURL url("ipns://QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, false));
  EXPECT_EQ(new_url,
            GURL(base::StrCat(
                {ipfs::GetDefaultIPFSGateway().spec(),
                 "ipns/QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd"})));
}

TEST_F(IpfsUtilsUnitTest, RFC3986TranslateIPFSURIIPFSSchemePublic) {
  GURL url("ipfs:QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, false));
  EXPECT_EQ(new_url,
            GURL(base::StrCat(
                {ipfs::GetDefaultIPFSGateway().spec(),
                 "ipfs/QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG"})));
}

TEST_F(IpfsUtilsUnitTest, RFC3986TranslateIPFSURIIPNSSchemePublic) {
  GURL url("ipns:QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, false));
  EXPECT_EQ(new_url,
            GURL(base::StrCat(
                {ipfs::GetDefaultIPFSGateway().spec(),
                 "ipns/QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG"})));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeWithPath) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, false));
  EXPECT_EQ(
      new_url,
      GURL(base::StrCat(
          {ipfs::GetDefaultIPFSGateway().spec(),
           "ipfs/bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq",
           "/wiki/Vincent_van_Gogh.html"})));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeWithPathAndHash) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html#Emerging_artist");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, false));
  EXPECT_EQ(
      new_url,
      GURL(base::StrCat(
          {ipfs::GetDefaultIPFSGateway().spec(),
           "ipfs/bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq",
           "/wiki/Vincent_van_Gogh.html#Emerging_artist"})));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeWithPathAndQuery) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html?test=true");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, false));
  EXPECT_EQ(
      new_url,
      GURL(base::StrCat(
          {ipfs::GetDefaultIPFSGateway().spec(),
           "ipfs/bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq",
           "/wiki/Vincent_van_Gogh.html?test=true"})));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeWithPathQueryHash) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html?test=true#test");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, false));
  EXPECT_EQ(
      new_url,
      GURL(base::StrCat(
          {ipfs::GetDefaultIPFSGateway().spec(),
           "ipfs/bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq",
           "/wiki/Vincent_van_Gogh.html?test=true#test"})));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURINotIPFSSchemeSubdomain) {
  GURL url(
      "http://a.com/ipfs/bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsgg"
      "enkbw6slwk4");
  GURL new_url;
  ASSERT_FALSE(ipfs::TranslateIPFSURI(url, &new_url, true));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeSubdomain) {
  GURL url(
      "ipfs://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkbw6slwk4");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, true));
  EXPECT_EQ(new_url,
            GURL(base::StrCat(
                {"https://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkb",
                 "w6slwk4.ipfs.", ipfs::GetDefaultIPFSGateway().host(), "/"})));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPNSSchemeSubdomain) {
  GURL url(
      "ipns://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkbw6slwk4");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, true));
  EXPECT_EQ(new_url,
            GURL(base::StrCat(
                {"https://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkb"
                 "w6slwk4.ipns.",
                 ipfs::GetDefaultIPFSGateway().host(), "/"})));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeWithPathSubdomain) {
  GURL url(
      "ipfs://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkbw6slwk4"
      "/wiki/Vincent_van_Gogh.html");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, true));
  EXPECT_EQ(new_url,
            GURL(base::StrCat({
                "https://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkb",
                "w6slwk4.ipfs.",
                ipfs::GetDefaultIPFSGateway().host(),
                "/wiki/Vincent_van_Gogh.html",
            })));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeWithPathAndHashSubdomain) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html#Emerging_artist");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, true));
  EXPECT_EQ(new_url,
            GURL(base::StrCat(
                {"https://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3ev",
                 "fyavhwq.ipfs.", ipfs::GetDefaultIPFSGateway().host(),
                 "/wiki/Vincent_van_Gogh.html#Emerging_artist"})));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeWithPathAndQuerySubdomain) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html?test=true");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, true));
  EXPECT_EQ(
      new_url,
      GURL(base::StrCat(
          {"https://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evf",
           "yavhwq.ipfs.", ipfs::GetDefaultIPFSGateway().host(),
           "/wiki/Vincent_van_Gogh.html?test=true"})));
}

TEST_F(IpfsUtilsUnitTest,
       TranslateIPFSURIIPFSSchemeWithPathQueryHashSubdomain) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html?test=true#test");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, true));
  EXPECT_EQ(new_url,
            GURL(base::StrCat(
                {"https://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3ev",
                 "fyavhwq.ipfs.", ipfs::GetDefaultIPFSGateway().host(),
                 "/wiki/Vincent_van_Gogh.html?test=true", "#test"})));
}

TEST_F(IpfsUtilsUnitTest, ContentHashToIpfsTest) {
  std::string contenthash =
      "e30101701220f073be187e8e06039796c432a"
      "5bdd6da3f403c2f93fa5d9dbdc5547c7fe0e3bc";
  std::vector<uint8_t> hex;
  base::HexStringToBytes(contenthash, &hex);
  GURL ipfs_url = ipfs::ContentHashToCIDv1URL(hex);
  ASSERT_TRUE(ipfs_url.is_valid());
  EXPECT_EQ(
      ipfs_url.spec(),
      "ipfs://bafybeihqoo7bq7uoaybzpfwegks33vw2h5adyl4t7joz3pofkr6h7yhdxq");

  contenthash =
      "e50101701220f073be187e8e06039796c432a"
      "5bdd6da3f403c2f93fa5d9dbdc5547c7fe0e3bc";
  hex.clear();
  base::HexStringToBytes(contenthash, &hex);
  ipfs_url = ipfs::ContentHashToCIDv1URL(hex);
  ASSERT_TRUE(ipfs_url.is_valid());
  EXPECT_EQ(
      ipfs_url.spec(),
      "ipns://bafybeihqoo7bq7uoaybzpfwegks33vw2h5adyl4t7joz3pofkr6h7yhdxq");
  contenthash =
      "0101701220f073be187e8e06039796c432a"
      "5bdd6da3f403c2f93fa5d9dbdc5547c7fe0e3bc";
  hex.clear();
  base::HexStringToBytes(contenthash, &hex);
  ipfs_url = ipfs::ContentHashToCIDv1URL(hex);
  ASSERT_FALSE(ipfs_url.is_valid());
  EXPECT_EQ(ipfs_url.spec(), "");
}

TEST_F(IpfsUtilsUnitTest, ExtractSourceFromGateway) {
  {
    GURL url =
        ipfs::ExtractSourceFromGateway(
            GURL("https://ipfs.io/ipfs/"
                 "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"))
            .value();
    EXPECT_EQ(url, GURL("ipfs://"
                        "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfy"
                        "avhwq"));
  }

  {
    GURL url = ipfs::ExtractSourceFromGateway(
                   GURL("https://ipfs.io/ipfs//////"
                        "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfy"
                        "avhwq////p1////Index.html#ref"))
                   .value();
    EXPECT_EQ(url, GURL("ipfs://"
                        "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfy"
                        "avhwq/p1/Index.html#ref"));
  }

  {
    GURL url = ipfs::ExtractSourceFromGateway(
                   GURL("https://ipfs.io/ipfs////"
                        "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfy"
                        "avhwq////p1/Index.html?a=b#ref"))
                   .value();
    EXPECT_EQ(url, GURL("ipfs://"
                        "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfy"
                        "avhwq/p1/Index.html?a=b#ref"));
  }

  {
    GURL url = ipfs::ExtractSourceFromGateway(
                   GURL("https://"
                        "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfy"
                        "avhwq.ipfs.ipfs.io/p1/Index.html?a=b#ref"))
                   .value();
    EXPECT_EQ(url, GURL("ipfs://"
                        "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfy"
                        "avhwq/p1/Index.html?a=b#ref"));
  }

  {
    EXPECT_FALSE(ipfs::ExtractSourceFromGateway(
        GURL("https://"
             "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfy"
             "avhwq.abc.io")));
  }

  {
    EXPECT_FALSE(ipfs::ExtractSourceFromGateway(GURL("https://abc.io/ipfs/")));
  }
}

TEST_F(IpfsUtilsUnitTest, ExtractIPNSSourceFromGateway) {
  {
    GURL url =
        ipfs::ExtractSourceFromGateway(GURL("https://ipfs.io/ipns/"
                                            "en-wikipedia--on--ipfs-org"))
            .value();
    EXPECT_EQ(url, GURL("https://en.wikipedia-on-ipfs.org"));
  }

  {
    GURL url = ipfs::ExtractSourceFromGateway(
                   GURL("https://ipfs.io/ipns//////en.wikipedia-on-ipfs.org////"
                        "p1////Index.html#ref"))
                   .value();
    EXPECT_EQ(url, GURL("https://en.wikipedia-on-ipfs.org/p1/Index.html#ref"));
  }

  {
    GURL url = ipfs::ExtractSourceFromGateway(
                   GURL("https://ipfs.io/ipns////en-wikipedia--on--ipfs-org////"
                        "p1/Index.html?a=b#ref"))
                   .value();
    EXPECT_EQ(url,
              GURL("https://en.wikipedia-on-ipfs.org/p1/Index.html?a=b#ref"));
  }

  {
    GURL url =
        ipfs::ExtractSourceFromGateway(GURL("https://"
                                            "en-wikipedia--on--ipfs-org.ipns."
                                            "ipfs.io/p1/Index.html?a=b#ref"))
            .value();
    EXPECT_EQ(url,
              GURL("https://en.wikipedia-on-ipfs.org/p1/Index.html?a=b#ref"));
  }

  {
    GURL url =
        ipfs::ExtractSourceFromGateway(GURL("https://"
                                            "k51qzi5uqu5dlvj2baxnqndepeb86cbk3n"
                                            "g7n3i46uzyxzyqj2xjonzllnv0v8.ipns."
                                            "ipfs.io/p1/Index.html?a=b#ref"))
            .value();
    EXPECT_EQ(url, GURL("ipns://"
                        "k51qzi5uqu5dlvj2baxnqndepeb86cbk3ng7n3i46uzyxzyqj2xjon"
                        "zllnv0v8/p1/Index.html?a=b#ref"));
  }

  {
    GURL url =
        ipfs::ExtractSourceFromGateway(GURL("https://ipfs.io/ipns////"
                                            "k51qzi5uqu5dlvj2baxnqndepeb86cbk3n"
                                            "g7n3i46uzyxzyqj2xjonzllnv0v8////"
                                            "p1/Index.html?a=b#ref"))
            .value();
    EXPECT_EQ(url, GURL("ipns://"
                        "k51qzi5uqu5dlvj2baxnqndepeb86cbk3ng7n3i46uzyxzyqj2xjon"
                        "zllnv0v8/p1/Index.html?a=b#ref"));
  }
  { EXPECT_FALSE(ipfs::ExtractSourceFromGateway(GURL("https://abc.abc.io"))); }

  {
    EXPECT_FALSE(ipfs::ExtractSourceFromGateway(GURL("https://abc.io/ipns/")));
  }
}
