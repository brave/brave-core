/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_block_safebrowsing_urls.h"

#include <memory>
#include <string>
#include <vector>

#include "brave/browser/net/url_context.h"
#include "net/base/net_errors.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace {
const char kInvalidUrl[] = "https://no-thanks.invalid";
}  // namespace

void CheckUrl(const std::string& test_url,
              const char* expected_url,
              int expected_error) {
  GURL new_url;
  int rc = brave::OnBeforeURLRequest_BlockSafeBrowsingReportingURLs(
      GURL(test_url), &new_url);
  EXPECT_EQ(new_url, GURL(expected_url));
  EXPECT_EQ(rc, expected_error);
}

TEST(BraveBlockReportingUrlsHelperTest, PreserveNormalUrls) {
  const std::vector<std::string> normalUrls({
      "https://brave.com/",
      "https://safebrowsing.google.com/safebrowsing",
      "https://safebrowsing.google.com/safebrowsing/clientreport/crx-list-info",
      "https://safebrowsing.googleapis.com/v4",
      "https://sb-ssl.google.com/safebrowsing/clientreport/download",
  });

  for (const auto& url : normalUrls) {
    CheckUrl(url, "", net::OK);
  }
}

TEST(BraveBlockReportingUrlsHelperTest, CancelReportingUrl) {
  const std::vector<std::string> reportingUrls({
      "https://sb-ssl.google.com/safebrowsing/clientreport/chrome-cct",
      "https://sb-ssl.google.com/safebrowsing/clientreport/chrome-reset",
      "https://sb-ssl.google.com/safebrowsing/clientreport/chrome-sw-reporter",
      "https://sb-ssl.google.com/safebrowsing/clientreport/incident",
      "https://sb-ssl.google.com/safebrowsing/clientreport/login",
      "https://sb-ssl.google.com/safebrowsing/clientreport/phishing",
      "https://sb-ssl.google.com/safebrowsing/clientreport/malware-check",
      "https://safebrowsing.google.com/safebrowsing/uploads/app",
      "https://safebrowsing.google.com/safebrowsing/uploads/chrome",
      "https://safebrowsing.google.com/safebrowsing/uploads/scan",
      "https://safebrowsing.google.com/safebrowsing/uploads/webprotect",
      "https://safebrowsing.google.com/safebrowsing/report",
      "https://safebrowsing.google.com/safebrowsing/clientreport/malware",
      "https://safebrowsing.google.com/safebrowsing/uploads/chrome",
      "https://safebrowsing.google.com/safebrowsing/clientreport/realtime",
  });

  for (const auto& url : reportingUrls) {
    CheckUrl(url, kInvalidUrl, net::ERR_ABORTED);
  }
}
