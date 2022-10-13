/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>

#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace brave_wallet {

TEST(PermissionUtilsUnitTest, GetConcatOriginFromWalletAddresses) {
  struct {
    std::vector<std::string> addrs;
    const char* expected_out_origin;
    const char* expected_out_origin_with_port;
  } cases[] = {
      {{"0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A",
        "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B"},
       "https://"
       "test.com{addr=0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A&addr="
       "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B}",
       "https://"
       "test.com{addr=0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A&addr="
       "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B}:123"},
      {
          {"BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
           "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV"},
          "https://"
          "test.com{addr=BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8&addr="
          "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV}",
          "https://"
          "test.com{addr=BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8&addr="
          "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV}:123",
      }};

  url::Origin origin = url::Origin::Create(GURL("https://test.com"));
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    url::Origin out_origin;
    EXPECT_TRUE(
        GetConcatOriginFromWalletAddresses(origin, cases[i].addrs, &out_origin))
        << "case: " << i;
    EXPECT_EQ(out_origin,
              url::Origin::Create(GURL(cases[i].expected_out_origin)))
        << "case: " << i;

    EXPECT_FALSE(GetConcatOriginFromWalletAddresses(
        url::Origin(), cases[i].addrs, &out_origin))
        << "case: " << i;
    EXPECT_FALSE(GetConcatOriginFromWalletAddresses(
        origin, std::vector<std::string>(), &out_origin))
        << "case: " << i;

    // Origin with port case:
    EXPECT_TRUE(GetConcatOriginFromWalletAddresses(
        url::Origin::Create(GURL("https://test.com:123")), cases[i].addrs,
        &out_origin))
        << "case: " << i;
    EXPECT_EQ(out_origin,
              url::Origin::Create(GURL(cases[i].expected_out_origin_with_port)))
        << "case: " << i;
  }
}

TEST(PermissionUtilsUnitTest, ParseRequestingOriginFromSubRequest) {
  struct {
    permissions::RequestType type;
    const char* invalid_origin;
    const char* invalid_origin_with_path;
    const char* valid_origin;
    const char* valid_origin_with_port;
    const char* account;
  } cases[] = {
      {permissions::RequestType::kBraveEthereum, "https://test.com0x123",
       "https://test.com0x123/path",
       "https://test.com0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A",
       "https://test.com0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A:123",
       "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A"},
      {permissions::RequestType::kBraveSolana,
       "https://test.com--BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
       "https://test.com--BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8/path",
       "https://test.com__BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
       "https://test.com__BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8:123",
       "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8"}};

  url::Origin requesting_origin;
  std::string account;
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    // Invalid requesting_origin format:
    EXPECT_FALSE(ParseRequestingOriginFromSubRequest(
        cases[i].type, url::Origin::Create(GURL(cases[i].invalid_origin)),
        nullptr, nullptr))
        << "case: " << i;
    EXPECT_FALSE(ParseRequestingOriginFromSubRequest(
        cases[i].type,
        url::Origin::Create(GURL(cases[i].invalid_origin_with_path)), nullptr,
        nullptr))
        << "case: " << i;
    EXPECT_FALSE(ParseRequestingOriginFromSubRequest(
        cases[i].type, url::Origin(), nullptr, nullptr))
        << "case: " << i;
    // invalid type
    EXPECT_FALSE(ParseRequestingOriginFromSubRequest(
        permissions::RequestType::kGeolocation,
        url::Origin::Create(GURL(cases[i].valid_origin)), nullptr, nullptr))
        << "case: " << i;
    EXPECT_TRUE(ParseRequestingOriginFromSubRequest(
        cases[i].type, url::Origin::Create(GURL(cases[i].valid_origin)),
        &requesting_origin, &account))
        << "case: " << i;
    EXPECT_EQ(requesting_origin.GetURL(), GURL("https://test.com"));
    EXPECT_TRUE(base::EqualsCaseInsensitiveASCII(account, cases[i].account))
        << "case: " << i;

    EXPECT_TRUE(ParseRequestingOriginFromSubRequest(
        cases[i].type,
        url::Origin::Create(GURL(cases[i].valid_origin_with_port)),
        &requesting_origin, &account))
        << "case: " << i;
    EXPECT_EQ(requesting_origin.GetURL(), GURL("https://test.com:123"));
    EXPECT_TRUE(base::EqualsCaseInsensitiveASCII(account, cases[i].account))
        << "case: " << i;
  }

  // separator in domain would still work
  EXPECT_TRUE(ParseRequestingOriginFromSubRequest(
      permissions::RequestType::kBraveSolana,
      url::Origin::Create(GURL(
          "https://test__.com__BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8")),
      &requesting_origin, &account));
  EXPECT_EQ(requesting_origin.GetURL(), GURL("https://test__.com"));
  EXPECT_TRUE(base::EqualsCaseInsensitiveASCII(
      account, "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8"));
  EXPECT_TRUE(ParseRequestingOriginFromSubRequest(
      permissions::RequestType::kBraveSolana,
      url::Origin::Create(
          GURL("https://"
               "test__.com__BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8:123")),
      &requesting_origin, &account));
  EXPECT_EQ(requesting_origin.GetURL(), GURL("https://test__.com:123"));
  EXPECT_TRUE(base::EqualsCaseInsensitiveASCII(
      account, "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8"));
}

TEST(PermissionUtilsUnitTest, ParseRequestingOrigin) {
  struct {
    permissions::RequestType type;
    const char* invalid_origin;
    const char* invalid_origin_with_path;
    const char* valid_origin;
    const char* valid_origin_with_port;
    const char* valid_origin_two_accounts;
    const char* valid_origin_two_accounts_with_port;
    const char* account1;
    const char* account2;
  } cases[]{
      {permissions::RequestType::kBraveEthereum, "https://test.com0x123",
       "https://test.com0x123/path",
       "https://test.com{addr=0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A}",
       "https://test.com{addr=0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A}:123",
       "https://"
       "test.com{addr=0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A&addr="
       "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B}",
       "https://"
       "test.com{addr=0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A&addr="
       "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B}:123",
       "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A",
       "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B"},
      {permissions::RequestType::kBraveSolana,
       "https://test.com--BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
       "https://test.com__BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8/path",
       "https://test.com{addr=BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8}",
       "https://"
       "test.com{addr=BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8}:123",
       "https://"
       "test.com{addr=BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8&addr="
       "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV}",
       "https://"
       "test.com{addr=BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8&addr="
       "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV}:123",
       "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
       "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV"}};

  std::string account;
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    // Invalid requesting_origin format:
    EXPECT_FALSE(ParseRequestingOrigin(
        cases[i].type, url::Origin::Create(GURL(cases[i].invalid_origin)),
        nullptr, nullptr))
        << "case: " << i;
    EXPECT_FALSE(ParseRequestingOrigin(
        cases[i].type,
        url::Origin::Create(GURL(cases[i].invalid_origin_with_path)), nullptr,
        nullptr))
        << "case: " << i;
    EXPECT_FALSE(
        ParseRequestingOrigin(cases[i].type, url::Origin(), nullptr, nullptr))
        << "case: " << i;
    // invalid type
    EXPECT_FALSE(ParseRequestingOrigin(
        permissions::RequestType::kGeolocation,
        url::Origin::Create(GURL(cases[i].valid_origin)), nullptr, nullptr))
        << "case: " << i;

    std::queue<std::string> address_queue;

    // Origin without port:
    url::Origin requesting_origin;
    EXPECT_TRUE(ParseRequestingOrigin(
        cases[i].type, url::Origin::Create(GURL(cases[i].valid_origin)),
        &requesting_origin, &address_queue));
    EXPECT_EQ(requesting_origin.GetURL(), GURL("https://test.com"))
        << "case: " << i;
    EXPECT_EQ(address_queue.size(), 1u) << "case: " << i;
    EXPECT_TRUE(base::EqualsCaseInsensitiveASCII(address_queue.front(),
                                                 cases[i].account1))
        << "case: " << i;

    EXPECT_TRUE(ParseRequestingOrigin(
        cases[i].type,
        url::Origin::Create(GURL(cases[i].valid_origin_two_accounts)),
        &requesting_origin, nullptr))
        << "case: " << i;
    EXPECT_EQ(requesting_origin.GetURL(), GURL("https://test.com"))
        << "case: " << i;

    // Origin with port:
    EXPECT_TRUE(ParseRequestingOrigin(
        cases[i].type,
        url::Origin::Create(GURL(cases[i].valid_origin_with_port)),
        &requesting_origin, nullptr))
        << "case: " << i;
    EXPECT_EQ(requesting_origin.GetURL(), GURL("https://test.com:123"))
        << "case: " << i;

    EXPECT_FALSE(ParseRequestingOrigin(
        cases[i].type,
        url::Origin::Create(GURL(cases[i].valid_origin_two_accounts_with_port)),
        &requesting_origin, &address_queue))
        << "Non-empty address_queue param should return false. case: " << i;

    address_queue = std::queue<std::string>();
    EXPECT_TRUE(ParseRequestingOrigin(
        cases[i].type,
        url::Origin::Create(GURL(cases[i].valid_origin_two_accounts_with_port)),
        &requesting_origin, &address_queue))
        << "case: " << i;
    EXPECT_EQ(requesting_origin.GetURL(), GURL("https://test.com:123"))
        << "case: " << i;
    EXPECT_EQ(address_queue.size(), 2u) << "case: " << i;
    EXPECT_TRUE(base::EqualsCaseInsensitiveASCII(address_queue.front(),
                                                 cases[i].account1))
        << "case: " << i;
    address_queue.pop();
    EXPECT_TRUE(base::EqualsCaseInsensitiveASCII(address_queue.front(),
                                                 cases[i].account2))
        << "case: " << i;
  }
}

TEST(PermissionUtilsUnitTest, GetSubRequestOrigin) {
  url::Origin new_origin;
  url::Origin old_origin = url::Origin::Create(GURL("https://test.com"));
  url::Origin old_origin_with_port =
      url::Origin::Create(GURL("https://test.com:123"));

  struct {
    permissions::RequestType type;
    const char* account;
    const char* expected_new_origin;
    const char* expected_new_origin_with_port;
  } cases[] = {
      {permissions::RequestType::kBraveEthereum,
       "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B",
       "https://test.com0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B",
       "https://test.com0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B:123"},
      {permissions::RequestType::kBraveSolana,
       "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
       "https://test.com__BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
       "https://"
       "test.com__BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8:123"}};
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    EXPECT_FALSE(GetSubRequestOrigin(cases[i].type, url::Origin(),
                                     cases[i].account, &new_origin))
        << "case: " << i;
    // invalid type
    EXPECT_FALSE(GetSubRequestOrigin(permissions::RequestType::kGeolocation,
                                     old_origin, cases[i].account, &new_origin))
        << "case: " << i;
    EXPECT_FALSE(
        GetSubRequestOrigin(cases[i].type, old_origin, "", &new_origin))
        << "case: " << i;
    EXPECT_FALSE(GetSubRequestOrigin(cases[i].type, old_origin,
                                     cases[i].account, nullptr))
        << "case: " << i;

    EXPECT_TRUE(GetSubRequestOrigin(cases[i].type, old_origin, cases[i].account,
                                    &new_origin))
        << "case: " << i;
    EXPECT_EQ(new_origin,
              url::Origin::Create(GURL(cases[i].expected_new_origin)));
    EXPECT_TRUE(GetSubRequestOrigin(cases[i].type, old_origin_with_port,
                                    cases[i].account, &new_origin))
        << "case: " << i;
    EXPECT_EQ(new_origin,
              url::Origin::Create(GURL(cases[i].expected_new_origin_with_port)))
        << "case: " << i;
  }
}

TEST(PermissionUtilsUnitTest, GetConnectWithSiteWebUIURL) {
  GURL base_url("chrome://wallet-panel.top-chrome/");
  url::Origin origin = url::Origin::Create(GURL("https://a.test.com:123"));
  struct {
    std::vector<std::string> addrs;
    const char* expected_out_url;
  } cases[] = {
      {{"0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A",
        "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B"},
       "chrome://wallet-panel.top-chrome/"
       "?addr=0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8A&addr="
       "0xaf5Ad1E10926C0Ee4af4eDAC61DD60E853753f8B&origin-scheme=https&"
       "origin-host=a.test.com&origin-port=123&origin-spec=https://"
       "a.test.com:123&etld-plus-one=test.com#connectWithSite"},
      {{"BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8",
        "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV"},
       "chrome://wallet-panel.top-chrome/"
       "?addr=BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8&addr="
       "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV&origin-scheme=https&"
       "origin-host=a.test.com&origin-port=123&origin-spec=https://"
       "a.test.com:123&etld-plus-one=test.com#connectWithSite"}};
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    GURL url_out = GetConnectWithSiteWebUIURL(base_url, cases[i].addrs, origin);
    EXPECT_EQ(url_out.spec(), cases[i].expected_out_url) << "case: " << i;
  }
}

}  // namespace brave_wallet
