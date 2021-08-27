/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_SHIELDS_AD_BLOCK_SERVICE_BROWSERTEST_H_
#define BRAVE_BROWSER_BRAVE_SHIELDS_AD_BLOCK_SERVICE_BROWSERTEST_H_

#include <string>

#include "chrome/browser/extensions/extension_browsertest.h"

class HostContentSettingsMap;

class AdBlockServiceTest : public extensions::ExtensionBrowserTest {
 public:
  AdBlockServiceTest() {}

  // ExtensionBrowserTest overrides
  void SetUpOnMainThread() override;
  void SetUp() override;
  void PreRunTestOnMainThread() override;

 protected:
  HostContentSettingsMap* content_settings();
  void UpdateAdBlockInstanceWithRules(const std::string& rules,
                                      const std::string& resources = "");
  void AssertTagExists(const std::string& tag, bool expected_exists) const;
  void InitEmbeddedTestServer();
  void GetTestDataDir(base::FilePath* test_data_dir);
  void SetDefaultComponentIdAndBase64PublicKeyForTest();
  void SetRegionalComponentIdAndBase64PublicKeyForTest();
  bool InstallDefaultAdBlockExtension(
      const std::string& extension_dir = "adblock-default",
      int expected_change = 1);
  bool InstallRegionalAdBlockExtension(const std::string& uuid);
  bool StartAdBlockRegionalServices();
  void SetSubscriptionIntervals();
  void WaitForAdBlockServiceThreads();
  void WaitForBraveExtensionShieldsDataReady();
  void ShieldsDown(const GURL& url);
};

#endif  // BRAVE_BROWSER_BRAVE_SHIELDS_AD_BLOCK_SERVICE_BROWSERTEST_H_
