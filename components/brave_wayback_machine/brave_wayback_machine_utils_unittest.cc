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
      R"(https://www.example.com?&timestamp=20160101&callback={"archived_snapshots":{"closest":{"url":"https://example.com/favicon.ico"}}}//)";  // NOLINT
  constexpr char kCallbackParameter[] =
      R"({"archived_snapshots":{"closest":{"url":"https://example.com/favicon.ico"}}}//)";  // NOLINT
  GURL wayback_fetch_url(std::string(kWaybackQueryURL) + kTestURL);
  std::string timestamp_value;
  EXPECT_TRUE(net::GetValueForKeyInQuery(wayback_fetch_url, "timestamp",
                                         &timestamp_value));
  std::string callback_value;
  EXPECT_TRUE(net::GetValueForKeyInQuery(wayback_fetch_url, "callback",
                                         &callback_value));
  EXPECT_EQ("20160101", timestamp_value);
  EXPECT_EQ(kCallbackParameter, callback_value);

  wayback_fetch_url = FixupWaybackQueryURL(wayback_fetch_url);
  EXPECT_TRUE(net::GetValueForKeyInQuery(wayback_fetch_url, "timestamp",
                                         &timestamp_value));
  EXPECT_TRUE(net::GetValueForKeyInQuery(wayback_fetch_url, "callback",
                                         &callback_value));
  // Check value is empty.
  EXPECT_EQ("", timestamp_value);
  EXPECT_EQ("", callback_value);
}
