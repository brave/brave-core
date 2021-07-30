/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/components/brave_wayback_machine/brave_wayback_machine_utils.h"
#include "brave/components/brave_wayback_machine/url_constants.h"
#include "net/base/url_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

TEST(BraveWaybackMachineUtilsTest, LocalHostDisabledTest) {
  EXPECT_TRUE(
      IsWaybackMachineDisabledFor(GURL("https://web.archive.org/foobar.html")));
  EXPECT_TRUE(IsWaybackMachineDisabledFor(GURL("http://localhost/index.html")));
  EXPECT_TRUE(IsWaybackMachineDisabledFor(GURL("http://abcd.local")));
  EXPECT_TRUE(IsWaybackMachineDisabledFor(GURL("http://abcd.onion")));
  EXPECT_TRUE(IsWaybackMachineDisabledFor(GURL("http://127.0.0.1")));
  EXPECT_TRUE(IsWaybackMachineDisabledFor(GURL("http://[::1]")));
  EXPECT_TRUE(IsWaybackMachineDisabledFor(
      GURL("http://127.0045.1.2:8080/index.html")));
  EXPECT_FALSE(IsWaybackMachineDisabledFor(GURL("http://www.local-news.com")));
  EXPECT_FALSE(IsWaybackMachineDisabledFor(GURL("http://www.onion-news.com")));
  EXPECT_FALSE(IsWaybackMachineDisabledFor(GURL("http://www.brave.com")));
  EXPECT_FALSE(
      IsWaybackMachineDisabledFor(GURL("https://archive.org/foobar.html")));
}

TEST(BraveWaybackMachineUtilsTest, FixupQueryURLTest) {
  constexpr char kTestURL[] =
      R"(https://www.example.com?&query1=abcd&timestamp=20160101&query2=&callback={"archived_snapshots":{"closest":{"url":"https://example.com/favicon.ico"}}}//)";  // NOLINT
  constexpr char kCallbackParameter[] =
      R"({"archived_snapshots":{"closest":{"url":"https://example.com/favicon.ico"}}}//)";  // NOLINT
  constexpr char kCallbackKey[] = "callback";
  constexpr char kTimestampKey[] = "timestamp";
  constexpr char kQuery1Key[] = "query1";
  constexpr char kQuery2Key[] = "query2";
  GURL wayback_fetch_url(std::string(kWaybackQueryURL) + kTestURL);
  std::string timestamp_value;
  EXPECT_TRUE(net::GetValueForKeyInQuery(wayback_fetch_url, kTimestampKey,
                                         &timestamp_value));
  std::string callback_value;
  EXPECT_TRUE(net::GetValueForKeyInQuery(wayback_fetch_url, kCallbackKey,
                                         &callback_value));
  std::string query1_value;
  std::string query2_value;
  EXPECT_TRUE(
      net::GetValueForKeyInQuery(wayback_fetch_url, kQuery1Key, &query1_value));
  EXPECT_TRUE(
      net::GetValueForKeyInQuery(wayback_fetch_url, kQuery2Key, &query2_value));
  EXPECT_EQ("20160101", timestamp_value);
  EXPECT_EQ(kCallbackParameter, callback_value);
  EXPECT_EQ("abcd", query1_value);
  EXPECT_EQ("", query2_value);

  wayback_fetch_url = FixupWaybackQueryURL(wayback_fetch_url);
  EXPECT_FALSE(net::GetValueForKeyInQuery(wayback_fetch_url, kTimestampKey,
                                          &timestamp_value));
  EXPECT_FALSE(net::GetValueForKeyInQuery(wayback_fetch_url, kCallbackKey,
                                          &callback_value));
  // Check unrelated query is not touched.
  EXPECT_TRUE(
      net::GetValueForKeyInQuery(wayback_fetch_url, kQuery1Key, &query1_value));
  EXPECT_EQ("abcd", query1_value);
  EXPECT_TRUE(
      net::GetValueForKeyInQuery(wayback_fetch_url, kQuery2Key, &query2_value));
  EXPECT_EQ("", query2_value);

  // Uses encoded callback key(%63allback) in queries.
  constexpr char kTestURL2[] =
      R"(https://www.example.com?&timestamp=20160101&%63allback={"archived_snapshots":{"closest":{"url":"https://example.com/favicon.ico"}}}//)";  // NOLINT
  constexpr char kEncodedCallbackKey[] = R"(%63allback)";
  GURL wayback_fetch_url2(std::string(kWaybackQueryURL) + kTestURL2);
  EXPECT_TRUE(net::GetValueForKeyInQuery(wayback_fetch_url2,
                                         kEncodedCallbackKey, &callback_value));
  EXPECT_EQ(kCallbackParameter, callback_value);

  // Check key is not found.
  wayback_fetch_url2 = FixupWaybackQueryURL(wayback_fetch_url2);
  EXPECT_FALSE(net::GetValueForKeyInQuery(
      wayback_fetch_url2, kEncodedCallbackKey, &callback_value));
  EXPECT_FALSE(net::GetValueForKeyInQuery(wayback_fetch_url2, kCallbackKey,
                                          &callback_value));
}
