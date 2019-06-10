/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/tor/tor_profile_service.h"

#include <string>

#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

class TorProfileServiceTest : public testing::Test {
 public:
  TorProfileServiceTest() {}
  ~TorProfileServiceTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TorProfileServiceTest);
};

TEST_F(TorProfileServiceTest, CircuitIsolationKey) {
  const struct {
    GURL url;
    std::string key;
  } cases[] = {
      {
          GURL("https://1.1.1.1/"),
          "1.1.1.1",
      },
      {
          GURL("https://1.1.1.1:53/"),
          "1.1.1.1",
      },
      {
          GURL("https://127.0.0.1/"),
          "127.0.0.1",
      },
      {
          GURL("https://127.0.0.53/"),
          "127.0.0.53",
      },
      {
          GURL("https://8.8.8.8/"),
          "8.8.8.8",
      },
      {
          GURL("https://8.8.8.8:80/"),
          "8.8.8.8",
      },
      {
          GURL("https://[::1]/"),
          "[::1]",
      },
      {
          GURL("https://check.torproject.org/"),
          "torproject.org",
      },
      {
          GURL("https://check.torproject.org/x"),
          "torproject.org",
      },
      {
          GURL("https://check.torproject.org/x?y"),
          "torproject.org",
      },
      {
          GURL("https://check.torproject.org/x?y#z"),
          "torproject.org",
      },
      {
          GURL("https://localhost/"),
          "localhost",
      },
      {
          GURL("https://localhost:8888/"),
          "localhost",
      },
      {
          GURL("https://user:pass@localhost:8888/"),
          "localhost",
      },
      {
          GURL("https://www.bbc.co.uk/"),
          "bbc.co.uk",
      },
  };

  for (auto& c : cases) {
    const GURL& url = c.url;
    const std::string& expected_key = c.key;
    std::string actual_key = tor::TorProfileService::CircuitIsolationKey(url);

    EXPECT_EQ(expected_key, actual_key);
  }
}
