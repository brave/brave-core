/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_utils.h"

#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/test/task_environment.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_ports.h"
#include "brave/components/ipfs/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/version_info/channel.h"
#include "net/base/url_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

class IpfsUtilsUnitTest : public testing::Test {
 public:
  IpfsUtilsUnitTest() = default;
  ~IpfsUtilsUnitTest() override = default;

  void SetUp() override {
    prefs_.registry()->RegisterStringPref(kIPFSPublicGatewayAddress,
                                          ipfs::kDefaultIPFSGateway);
    prefs_.registry()->RegisterStringPref(kIPFSPublicNFTGatewayAddress,
                                          ipfs::kDefaultIPFSNFTGateway);
    prefs_.registry()->RegisterIntegerPref(
        kIPFSResolveMethod,
        static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_ASK));

    public_gateway_ = GURL(ipfs::kDefaultIPFSGateway);
    local_gateway_ = GURL("http://localhost:48080");
  }

  const GURL& local_gateway() { return local_gateway_; }
  const GURL& public_gateway() { return public_gateway_; }
  PrefService* prefs() { return &prefs_; }

  bool ValidatePeerAddress(const std::string& value,
                           const std::string& expected_id,
                           const std::string& expected_address) {
    std::string id;
    std::string address;
    bool result = ipfs::ParsePeerConnectionString(value, &id, &address);
    EXPECT_EQ(id, expected_id);
    EXPECT_EQ(address, expected_address);
    return result;
  }

  void SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes type) {
    prefs_.SetInteger(kIPFSResolveMethod, static_cast<int>(type));
  }

 private:
  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple prefs_;
  GURL local_gateway_;
  GURL public_gateway_;
};

TEST_F(IpfsUtilsUnitTest, CIDValidation) {
  ASSERT_TRUE(ipfs::IsValidCID(
      "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"));
  ASSERT_TRUE(
      ipfs::IsValidCID("QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG"));
  ASSERT_TRUE(
      ipfs::IsValidCID("zb2rhe5P4gXftAwvA4eXQ5HJwsER2owDyS9sKaQRRVQPn93bA"));
  ASSERT_TRUE(ipfs::IsValidCID(
      "k51qzi5uqu5dlvj2baxnqndepeb86cbk3ng7n3i46uzyxzyqj2xjonzllnv0v8"));
  ASSERT_TRUE(ipfs::IsValidCID("bafkqaaa"));

  ASSERT_FALSE(ipfs::IsValidCID("7testtesttest"));
  ASSERT_FALSE(
      ipfs::IsValidCID("zb2rhe5P4gXftAwvA4eXQ5HJwsER2owDyS9sKaQRRVQPn=3bA"));
  ASSERT_FALSE(ipfs::IsValidCID("brantly.eth"));
  ASSERT_FALSE(ipfs::IsValidCID(
      "ba.ybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"));
  ASSERT_FALSE(ipfs::IsValidCID(
      "ba-ybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"));
  ASSERT_FALSE(ipfs::IsValidCID(
      "ba%ybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"));
  ASSERT_FALSE(ipfs::IsValidCID(
      "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyav"));
  ASSERT_FALSE(ipfs::IsValidCID("QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLy"));
}

TEST_F(IpfsUtilsUnitTest, HasIPFSPath) {
  std::vector<GURL> ipfs_urls(
      {GURL("http://localhost:48080/ipfs/"
            "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/wiki/"
            "Vincent_van_Gogh.html"),
       GURL("http://localhost:48080/ipns/tr.wikipedia-on-ipfs.org/wiki/"
            "Anasayfa.html")});

  for (auto url : ipfs_urls) {
    EXPECT_TRUE(ipfs::HasIPFSPath(url)) << url;
  }
}

TEST_F(IpfsUtilsUnitTest, IsDefaultGatewayURL) {
  std::vector<GURL> gateway_urls(
      {GURL("https://dweb.link/ipfs/"
            "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/wiki/"
            "Vincent_van_Gogh.html"),
       GURL("https://"
            "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq."
            "ipfs.dweb.link/wiki/Vincent_van_Gogh.html"),
       GURL("https://dweb.link/ipns/tr.wikipedia-on-ipfs.org/wiki/"
            "Anasayfa.html")});

  std::vector<GURL> ipfs_urls(
      {GURL("http://localhost:48080/ipfs/"
            "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/wiki/"
            "Vincent_van_Gogh.html"),
       GURL("http://localhost:48080/ipns/tr.wikipedia-on-ipfs.org/wiki/"
            "Anasayfa.html"),
       GURL("ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
            "/wiki/Vincent_van_Gogh.html")});

  for (auto url : gateway_urls) {
    EXPECT_TRUE(ipfs::IsDefaultGatewayURL(url, prefs())) << url;
  }

  for (auto url : ipfs_urls) {
    EXPECT_FALSE(ipfs::IsDefaultGatewayURL(url, prefs())) << url;
  }
}

TEST_F(IpfsUtilsUnitTest, IsLocalGatewayURL) {
  std::vector<GURL> local_gateway_urls(
      {GURL("http://localhost:48080/ipfs/"
            "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/wiki/"
            "Vincent_van_Gogh.html"),
       GURL(
           "http://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq."
           "ipfs.localhost:48080//wiki/Vincent_van_Gogh.html"),
       GURL("http://127.0.0.1:48080/ipfs/"
            "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/wiki/"
            "Vincent_van_Gogh.html")});

  std::vector<GURL> non_local_gateway_urls(
      {GURL("https://dweb.link/ipfs/"
            "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/wiki/"
            "Vincent_van_Gogh.html"),
       GURL("ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
            "/wiki/Vincent_van_Gogh.html"),
       GURL("http://github.com/ipfs/go-ipfs")});

  for (auto url : local_gateway_urls) {
    EXPECT_TRUE(ipfs::IsLocalGatewayURL(url)) << url;
  }

  for (auto url : non_local_gateway_urls) {
    EXPECT_FALSE(ipfs::IsLocalGatewayURL(url)) << url;
  }
}

TEST_F(IpfsUtilsUnitTest, ToPublicGatewayURL) {
  std::vector<GURL> ipfs_urls(
      {GURL("http://localhost:48080/ipfs/"
            "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/wiki/"
            "Vincent_van_Gogh.html"),
       GURL("http://127.0.0.1:48080/ipfs/"
            "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/wiki/"
            "Vincent_van_Gogh.html"),
       GURL("ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
            "/wiki/Vincent_van_Gogh.html")});

  GURL expected_new_url = GURL(
      "https://dweb.link/ipfs/"
      "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/wiki/"
      "Vincent_van_Gogh.html");

  for (auto url : ipfs_urls) {
    GURL new_url = ipfs::ToPublicGatewayURL(url, prefs());
    EXPECT_EQ(new_url, expected_new_url) << url;
  }
}

TEST_F(IpfsUtilsUnitTest, GetIPFSGatewayURL) {
  EXPECT_EQ(
      ipfs::GetIPFSGatewayURL(
          "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq", "",
          ipfs::GetDefaultIPFSGateway(prefs())),
      GURL(
          "https://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq."
          "ipfs.dweb.link"));
  EXPECT_EQ(
      ipfs::GetIPFSGatewayURL(
          "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq", "",
          ipfs::GetDefaultIPFSGateway(prefs())),
      GURL(
          "https://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq."
          "ipfs.dweb.link"));
}

TEST_F(IpfsUtilsUnitTest, GetIPFSGatewayURLLocal) {
  EXPECT_EQ(
      ipfs::GetIPFSGatewayURL(
          "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq", "",
          ipfs::GetDefaultIPFSLocalGateway(version_info::Channel::UNKNOWN)),
      GURL("http://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq."
           "ipfs.localhost:48080"));
  EXPECT_EQ(
      ipfs::GetIPFSGatewayURL(
          "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq", "",
          ipfs::GetDefaultIPFSLocalGateway(version_info::Channel::UNKNOWN)),
      GURL("http://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq."
           "ipfs.localhost:48080"));
}

TEST_F(IpfsUtilsUnitTest, IsLocalGatewayConfigured) {
  ASSERT_FALSE(ipfs::IsLocalGatewayConfigured(prefs()));
  SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL);
  ASSERT_TRUE(ipfs::IsLocalGatewayConfigured(prefs()));
}

TEST_F(IpfsUtilsUnitTest, GetConfiguredBaseGateway) {
  GURL url =
      ipfs::GetConfiguredBaseGateway(prefs(), version_info::Channel::UNKNOWN);
  ASSERT_EQ(url, GURL("https://dweb.link/"));
  SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL);
  url = ipfs::GetConfiguredBaseGateway(prefs(), version_info::Channel::UNKNOWN);
  ASSERT_EQ(url, GURL("http://localhost:48080/"));
}

TEST_F(IpfsUtilsUnitTest, ResolveIPFSURI) {
  GURL url =
      ipfs::GetConfiguredBaseGateway(prefs(), version_info::Channel::UNKNOWN);
  GURL gateway_url;
  ASSERT_TRUE(ipfs::ResolveIPFSURI(prefs(), version_info::Channel::UNKNOWN,
                                   GURL("ipfs://bafybeiemxf5abjwjbikoz4mc3a3dl"
                                        "a6ual3jsgpdr4cjr3oz3evfyavhwq"),
                                   &gateway_url));
  ASSERT_EQ(gateway_url, GURL("https://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsg"
                              "pdr4cjr3oz3evfyavhwq.ipfs.dweb.link"));
  SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL);
  ASSERT_TRUE(ipfs::ResolveIPFSURI(prefs(), version_info::Channel::UNKNOWN,
                                   GURL("ipfs://bafybeiemxf5abjwjbikoz4mc3a3dl"
                                        "a6ual3jsgpdr4cjr3oz3evfyavhwq"),
                                   &gateway_url));
  ASSERT_EQ(gateway_url, GURL("http://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgp"
                              "dr4cjr3oz3evfyavhwq.ipfs.localhost:48080"));
}

TEST_F(IpfsUtilsUnitTest, GetDefaultIPFSNFTGateway) {
  prefs()->SetString(kIPFSPublicNFTGatewayAddress, "https://example.com/");
  EXPECT_EQ(ipfs::GetDefaultNFTIPFSGateway(prefs()),
            GURL("https://example.com/"));
  prefs()->SetString(kIPFSPublicNFTGatewayAddress, "https://127.0.0.1:8888/");
  EXPECT_EQ(ipfs::GetDefaultNFTIPFSGateway(prefs()),
            GURL("https://localhost:8888/"));
  prefs()->SetString(kIPFSPublicNFTGatewayAddress, "https://127.0.0.1/");
  EXPECT_EQ(ipfs::GetDefaultNFTIPFSGateway(prefs()),
            GURL("https://localhost/"));
  prefs()->SetString(kIPFSPublicNFTGatewayAddress, "https://localhost/");
  EXPECT_EQ(ipfs::GetDefaultNFTIPFSGateway(prefs()),
            GURL("https://localhost/"));
}

TEST_F(IpfsUtilsUnitTest, GetDefaultIPFSGateway) {
  prefs()->SetString(kIPFSPublicGatewayAddress, "https://example.com/");
  EXPECT_EQ(ipfs::GetDefaultIPFSGateway(prefs()), GURL("https://example.com/"));
  prefs()->SetString(kIPFSPublicGatewayAddress, "https://127.0.0.1:8888/");
  EXPECT_EQ(ipfs::GetDefaultIPFSGateway(prefs()),
            GURL("https://localhost:8888/"));
  prefs()->SetString(kIPFSPublicGatewayAddress, "https://127.0.0.1/");
  EXPECT_EQ(ipfs::GetDefaultIPFSGateway(prefs()), GURL("https://localhost/"));
  prefs()->SetString(kIPFSPublicGatewayAddress, "https://localhost/");
  EXPECT_EQ(ipfs::GetDefaultIPFSGateway(prefs()), GURL("https://localhost/"));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURINotIPFSScheme) {
  GURL url("http://a.com/ipfs/QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  GURL new_url;
  ASSERT_FALSE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), false));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSScheme) {
  GURL url("ipfs://QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), false));
  EXPECT_EQ(new_url, GURL("https://dweb.link/ipfs/"
                          "QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG"));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPNSScheme) {
  GURL url("ipns://QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), false));
  EXPECT_EQ(new_url, GURL("https://dweb.link/ipns/"
                          "QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd"));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeLocal) {
  GURL url("ipfs://QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, local_gateway(), false));
  EXPECT_EQ(new_url, GURL("http://localhost:48080/ipfs/"
                          "QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG"));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPNSSchemeLocal) {
  GURL url("ipns://QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, local_gateway(), false));
  EXPECT_EQ(new_url, GURL("http://localhost:48080/ipns/"
                          "QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd"));
}

TEST_F(IpfsUtilsUnitTest, RFC3986TranslateIPFSURIIPFSSchemeLocal) {
  GURL url("ipfs:QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, local_gateway(), false));
  EXPECT_EQ(new_url, GURL("http://localhost:48080/ipfs/"
                          "QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG"));
}

TEST_F(IpfsUtilsUnitTest, RFC3986TranslateIPFSURIIPFSSchemePublic) {
  GURL url("ipfs:QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), false));
  EXPECT_EQ(new_url, GURL("https://dweb.link/ipfs/"
                          "QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG"));
}

TEST_F(IpfsUtilsUnitTest, RFC3986TranslateIPFSURIIPNSSchemeLocal) {
  GURL url("ipns:QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, local_gateway(), false));
  EXPECT_EQ(new_url, GURL("http://localhost:48080/ipns/"
                          "QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG"));
}

TEST_F(IpfsUtilsUnitTest, RFC3986TranslateIPFSURIIPNSSchemePublic) {
  GURL url("ipns:QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), false));
  EXPECT_EQ(new_url, GURL("https://dweb.link/ipns/"
                          "QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG"));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeWithPath) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), false));
  EXPECT_EQ(new_url,
            GURL("https://dweb.link/ipfs/"
                 "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
                 "/wiki/Vincent_van_Gogh.html"));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeWithPathAndHash) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html#Emerging_artist");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), false));
  EXPECT_EQ(new_url,
            GURL("https://dweb.link/ipfs/"
                 "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
                 "/wiki/Vincent_van_Gogh.html#Emerging_artist"));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeLocalWithPathAndHash) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html#Emerging_artist");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, local_gateway(), false));
  EXPECT_EQ(new_url,
            GURL("http://localhost:48080/ipfs/"
                 "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
                 "/wiki/Vincent_van_Gogh.html#Emerging_artist"));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeWithPathAndQuery) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html?test=true");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), false));
  EXPECT_EQ(new_url,
            GURL("https://dweb.link/ipfs/"
                 "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
                 "/wiki/Vincent_van_Gogh.html?test=true"));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeLocalWithPathAndQuery) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html?test=true");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, local_gateway(), false));
  EXPECT_EQ(new_url,
            GURL("http://localhost:48080/ipfs/"
                 "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
                 "/wiki/Vincent_van_Gogh.html?test=true"));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeWithPathQueryHash) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html?test=true#test");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), false));
  EXPECT_EQ(new_url,
            GURL("https://dweb.link/ipfs/"
                 "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
                 "/wiki/Vincent_van_Gogh.html?test=true#test"));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeLocalWithPathQueryHash) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html?test=true#test");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, local_gateway(), false));
  EXPECT_EQ(new_url,
            GURL("http://localhost:48080/ipfs/"
                 "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
                 "/wiki/Vincent_van_Gogh.html?test=true#test"));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURINotIPFSSchemeSubdomain) {
  GURL url(
      "http://a.com/ipfs/bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsgg"
      "enkbw6slwk4");
  GURL new_url;
  ASSERT_FALSE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), true));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeSubdomain) {
  GURL url(
      "ipfs://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkbw6slwk4");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), true));
  EXPECT_EQ(new_url,
            GURL("https://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkb"
                 "w6slwk4.ipfs.dweb.link/"));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPNSSchemeSubdomain) {
  GURL url(
      "ipns://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkbw6slwk4");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), true));
  EXPECT_EQ(new_url,
            GURL("https://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkb"
                 "w6slwk4.ipns.dweb.link/"));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeLocalSubdomain) {
  GURL url(
      "ipfs://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkbw6slwk4");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, local_gateway(), true));
  EXPECT_EQ(new_url,
            GURL("http://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkbw"
                 "6slwk4.ipfs.localhost:48080/"));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPNSSchemeLocalSubdomain) {
  GURL url(
      "ipns://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkbw6slwk4");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, local_gateway(), true));
  EXPECT_EQ(new_url, GURL("http://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanoko"
                          "nsggenkbw6slwk4.ipns.localhost:48080/"));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeWithPathSubdomain) {
  GURL url(
      "ipfs://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkbw6slwk4"
      "/wiki/Vincent_van_Gogh.html");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), true));
  EXPECT_EQ(new_url,
            GURL("https://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkb"
                 "w6slwk4.ipfs.dweb.link/wiki/Vincent_van_Gogh.html"));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeWithPathAndHashSubdomain) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html#Emerging_artist");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), true));
  EXPECT_EQ(
      new_url,
      GURL(
          "https://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3ev"
          "fyavhwq.ipfs.dweb.link/wiki/Vincent_van_Gogh.html#Emerging_artist"));
}

TEST_F(IpfsUtilsUnitTest,
       TranslateIPFSURIIPFSSchemeLocalWithPathAndHashSubdomain) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html#Emerging_artist");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, local_gateway(), true));
  EXPECT_EQ(new_url,
            GURL("http://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evf"
                 "yavhwq.ipfs.localhost:48080/wiki/Vincent_van_Gogh.html"
                 "#Emerging_artist"));
}

TEST_F(IpfsUtilsUnitTest, TranslateIPFSURIIPFSSchemeWithPathAndQuerySubdomain) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html?test=true");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), true));
  EXPECT_EQ(new_url,
            GURL("https://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evf"
                 "yavhwq.ipfs.dweb.link/wiki/Vincent_van_Gogh.html?test=true"));
}

TEST_F(IpfsUtilsUnitTest,
       TranslateIPFSURIIPFSSchemeLocalWithPathAndQuerySubdomain) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html?test=true");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, local_gateway(), true));
  EXPECT_EQ(new_url,
            GURL("http://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evf"
                 "yavhwq.ipfs.localhost:48080/wiki/Vincent_van_Gogh.html"
                 "?test=true"));
}

TEST_F(IpfsUtilsUnitTest,
       TranslateIPFSURIIPFSSchemeWithPathQueryHashSubdomain) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html?test=true#test");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), true));
  EXPECT_EQ(new_url,
            GURL("https://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3ev"
                 "fyavhwq.ipfs.dweb.link/wiki/Vincent_van_Gogh.html?test=true"
                 "#test"));
}

TEST_F(IpfsUtilsUnitTest,
       TranslateIPFSURIIPFSSchemeLocalWithPathQueryHashSubdomain) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html?test=true#test");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, local_gateway(), true));
  EXPECT_EQ(new_url,
            GURL("http://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evf"
                 "yavhwq.ipfs.localhost:48080/wiki/Vincent_van_Gogh.html"
                 "?test=true#test"));
}

TEST_F(IpfsUtilsUnitTest, ResolveWebUIFilesLocation) {
  GURL url = ipfs::ResolveWebUIFilesLocation("/test_directory",
                                             version_info::Channel::UNKNOWN);
  GURL api_server = ipfs::GetAPIServer(version_info::Channel::UNKNOWN);
  EXPECT_EQ(url.host(), api_server.host());
  EXPECT_EQ(url.path(), "/webui/");
  EXPECT_EQ(url.ref(), "/files/test_directory");
}

TEST_F(IpfsUtilsUnitTest, IsIpfsMenuEnabled) {
  ASSERT_FALSE(ipfs::IsLocalGatewayConfigured(prefs()));
  ASSERT_FALSE(ipfs::IsIpfsMenuEnabled(prefs()));
  SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL);
  ASSERT_TRUE(ipfs::IsLocalGatewayConfigured(prefs()));
  ASSERT_TRUE(ipfs::IsIpfsMenuEnabled(prefs()));
}

TEST_F(IpfsUtilsUnitTest, ParsePeerConnectionStringTest) {
  std::string id;
  std::string address;
  ASSERT_FALSE(ipfs::ParsePeerConnectionString("test", nullptr, nullptr));
  ASSERT_FALSE(ipfs::ParsePeerConnectionString("test", nullptr, &address));
  ASSERT_FALSE(ipfs::ParsePeerConnectionString("test", &id, nullptr));
  ASSERT_TRUE(id.empty());
  ASSERT_TRUE(address.empty());

  std::string value =
      "/ip4/104.131.131.82/udp/4001/quic/p2p/"
      "QmaCpDMGvV2BGHeYERUEnRQAwe3N8SzbUtfsmvsqQLuvuJ";
  ASSERT_TRUE(ValidatePeerAddress(
      value, "QmaCpDMGvV2BGHeYERUEnRQAwe3N8SzbUtfsmvsqQLuvuJ",
      "/ip4/104.131.131.82/udp/4001/quic"));

  value = "/p2p/QmaCpDMGvV2BGHeYERUEnRQAwe3N8SzbUtfsmvsqQLuvuJ";
  ASSERT_TRUE(ValidatePeerAddress(
      value, "QmaCpDMGvV2BGHeYERUEnRQAwe3N8SzbUtfsmvsqQLuvuJ", ""));

  value = "QmaCpDMGvV2BGHeYERUEnRQAwe3N8SzbUtfsmvsqQLuvuJ";
  ASSERT_TRUE(ValidatePeerAddress(
      value, "QmaCpDMGvV2BGHeYERUEnRQAwe3N8SzbUtfsmvsqQLuvuJ", ""));
  value = "12D3KooWBdmLJjhpgJ9KZgLM3f894ff9xyBfPvPjFNn7MKJpyrC2";
  ASSERT_FALSE(ValidatePeerAddress(value, "", ""));
  value =
      "/ip4/46.21.210.45/udp/14406/quic/p2p/"
      "12D3KooWBdmLJjhpgJ9KZgLM3f894ff9xyBfPvPjFNn7MKJpyrC2";
  ASSERT_TRUE(ValidatePeerAddress(
      value, "12D3KooWBdmLJjhpgJ9KZgLM3f894ff9xyBfPvPjFNn7MKJpyrC2",
      "/ip4/46.21.210.45/udp/14406/quic"));
}

TEST_F(IpfsUtilsUnitTest, ValidateNodeFilename) {
  ASSERT_TRUE(ipfs::IsValidNodeFilename("go-ipfs_v0.9.0-rc1_windows-amd64"));
  ASSERT_TRUE(ipfs::IsValidNodeFilename("go-ipfs_v0.9.0-rc21_windows-amd64"));
  ASSERT_TRUE(ipfs::IsValidNodeFilename("go-ipfs_v0.9.0_windows-amd64"));

  ASSERT_TRUE(ipfs::IsValidNodeFilename("go-ipfs_v0.9.0-rc1_darwin-amd64"));
  ASSERT_TRUE(ipfs::IsValidNodeFilename("go-ipfs_v0.9.0-rc21_darwin-amd64"));
  ASSERT_TRUE(ipfs::IsValidNodeFilename("go-ipfs_v0.9.0_darwin-amd64"));

  ASSERT_TRUE(ipfs::IsValidNodeFilename("go-ipfs_v0.9.0-rc1_darwin-arm64"));
  ASSERT_TRUE(ipfs::IsValidNodeFilename("go-ipfs_v0.9.0-rc21_darwin-arm64"));
  ASSERT_TRUE(ipfs::IsValidNodeFilename("go-ipfs_v0.9.0_darwin-arm64"));

  ASSERT_TRUE(ipfs::IsValidNodeFilename("go-ipfs_v0.9.0-rc1_linux-amd64"));
  ASSERT_TRUE(ipfs::IsValidNodeFilename("go-ipfs_v0.9.0-rc21_linux-amd64"));
  ASSERT_TRUE(ipfs::IsValidNodeFilename("go-ipfs_v0.9.0_linux-amd64"));

  ASSERT_FALSE(ipfs::IsValidNodeFilename(""));
  ASSERT_FALSE(ipfs::IsValidNodeFilename("ipfs.exe"));
  ASSERT_FALSE(ipfs::IsValidNodeFilename("go-ipfs_v0.9.0_linux"));
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

TEST_F(IpfsUtilsUnitTest, IsAPIGatewayTest) {
  auto channel = version_info::Channel::UNKNOWN;
  GURL api_server = ipfs::GetAPIServer(channel);
  ASSERT_TRUE(ipfs::IsAPIGateway(api_server, channel));
  ASSERT_TRUE(net::IsLocalhost(api_server));
  auto port = ipfs::GetAPIPort(channel);
  ASSERT_TRUE(ipfs::IsAPIGateway(GURL("https://127.0.0.1:" + port), channel));
  ASSERT_TRUE(ipfs::IsAPIGateway(GURL("https://localhost:" + port), channel));
  ASSERT_FALSE(ipfs::IsAPIGateway(GURL("https://brave.com"), channel));
  ASSERT_FALSE(ipfs::IsAPIGateway(GURL(), channel));
}

TEST_F(IpfsUtilsUnitTest, IPNSRegistryDomain) {
  EXPECT_EQ(ipfs::GetRegistryDomainFromIPNS(GURL("http://google.com")), "");
  EXPECT_EQ(ipfs::GetRegistryDomainFromIPNS(GURL("https://google.com")), "");
  EXPECT_EQ(ipfs::GetRegistryDomainFromIPNS(GURL("ipfs://bafy")), "");
  EXPECT_EQ(ipfs::GetRegistryDomainFromIPNS(GURL("ipfs://QmfdSDf")), "");
  EXPECT_EQ(ipfs::GetRegistryDomainFromIPNS(GURL("ipns://QmfdSDf/path")), "");
  EXPECT_EQ(ipfs::GetRegistryDomainFromIPNS(GURL("ipns://bafyff/path")), "");
  EXPECT_EQ(ipfs::GetRegistryDomainFromIPNS(GURL("ipns://brantly.eth.link")),
            "brantly.eth.link");
  EXPECT_EQ(ipfs::GetRegistryDomainFromIPNS(GURL("ipns://brantly.eth/path")),
            "brantly.eth");
  EXPECT_EQ(ipfs::GetRegistryDomainFromIPNS(GURL("ipns://blah.google.com")),
            "google.com");
}

TEST_F(IpfsUtilsUnitTest, IsValidCIDOrDomain) {
  ASSERT_TRUE(ipfs::IsValidCIDOrDomain(
      "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"));
  ASSERT_TRUE(ipfs::IsValidCIDOrDomain(
      "QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG"));
  ASSERT_TRUE(ipfs::IsValidCIDOrDomain(
      "zb2rhe5P4gXftAwvA4eXQ5HJwsER2owDyS9sKaQRRVQPn93bA"));
  ASSERT_TRUE(ipfs::IsValidCIDOrDomain("bafkqaaa"));
  ASSERT_TRUE(ipfs::IsValidCIDOrDomain(
      "k51qzi5uqu5dlvj2baxnqndepeb86cbk3ng7n3i46uzyxzyqj2xjonzllnv0v8"));
  ASSERT_FALSE(ipfs::IsValidCIDOrDomain("7testtesttest"));

  ASSERT_FALSE(ipfs::IsValidCIDOrDomain(
      "zb2rhe5P4gXftAwvA4eXQ5HJwsER2owDyS9sKaQRRVQPn=3bA"));
  ASSERT_TRUE(ipfs::IsValidCIDOrDomain("brantly.eth"));
  ASSERT_FALSE(ipfs::IsValidCIDOrDomain(
      "ba-ybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"));
  ASSERT_FALSE(ipfs::IsValidCIDOrDomain(
      "ba%ybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"));

  ASSERT_TRUE(ipfs::IsValidCIDOrDomain("test.com"));
  ASSERT_TRUE(ipfs::IsValidCIDOrDomain("test.net"));
  ASSERT_TRUE(ipfs::IsValidCIDOrDomain("a.b.c.com"));
  ASSERT_TRUE(ipfs::IsValidCIDOrDomain("a.b.c.localhost"));
  ASSERT_FALSE(ipfs::IsValidCIDOrDomain("a.b.c.com:11112"));
  ASSERT_FALSE(ipfs::IsValidCIDOrDomain("wrongdomainandcid"));
}

TEST_F(IpfsUtilsUnitTest, TranslateXIPFSPath) {
  ASSERT_FALSE(ipfs::TranslateXIPFSPath(""));
  ASSERT_FALSE(ipfs::TranslateXIPFSPath("abc"));
  ASSERT_FALSE(ipfs::TranslateXIPFSPath("ipfs/abc"));
  ASSERT_FALSE(ipfs::TranslateXIPFSPath("ipns/abc"));
  ASSERT_FALSE(ipfs::TranslateXIPFSPath("/ipfsabc"));
  ASSERT_FALSE(ipfs::TranslateXIPFSPath("/ipnsabc"));
  ASSERT_EQ(GURL("ipfs://abc"), ipfs::TranslateXIPFSPath("/ipfs/abc"));
  ASSERT_EQ(GURL("ipns://abc"), ipfs::TranslateXIPFSPath("/ipns/abc"));
  ASSERT_FALSE(ipfs::TranslateXIPFSPath("/ipfs/"));
  ASSERT_FALSE(ipfs::TranslateXIPFSPath("/ipns/"));
}

TEST_F(IpfsUtilsUnitTest, DecodeSingleLabelForm) {
  EXPECT_EQ("en.wikipedia-on-ipfs.org",
            ipfs::DecodeSingleLabelForm("en-wikipedia--on--ipfs-org"));
  EXPECT_EQ("a-b.c-d", ipfs::DecodeSingleLabelForm("a--b-c--d"));
  EXPECT_EQ("en.wikipedia-on-ipfs.org",
            ipfs::DecodeSingleLabelForm("en.wikipedia-on-ipfs.org"));
  EXPECT_EQ("", ipfs::DecodeSingleLabelForm(""));
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
