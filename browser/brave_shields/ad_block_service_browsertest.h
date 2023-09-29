/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_SHIELDS_AD_BLOCK_SERVICE_BROWSERTEST_H_
#define BRAVE_BROWSER_BRAVE_SHIELDS_AD_BLOCK_SERVICE_BROWSERTEST_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_shields/content/test/test_filters_provider.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/test/spawned_test_server/spawned_test_server.h"

class HostContentSettingsMap;

namespace brave_shields {
class AdBlockService;
}  // namespace brave_shields

class AdBlockServiceTest : public InProcessBrowserTest {
 public:
  AdBlockServiceTest();
  ~AdBlockServiceTest() override;

  // ExtensionBrowserTest overrides
  void SetUpCommandLine(base::CommandLine* command_line) override;
  void SetUpInProcessBrowserTestFixture() override;
  void SetUpOnMainThread() override;
  void SetUp() override;
  void PreRunTestOnMainThread() override;
  void TearDownOnMainThread() override;
  void TearDownInProcessBrowserTestFixture() override;

 protected:
  content::ContentMockCertVerifier mock_cert_verifier_;

  HostContentSettingsMap* content_settings();
  void UpdateAdBlockInstanceWithRules(const std::string& rules,
                                      const std::string& resources = "[]",
                                      uint8_t permission_mask = 0);
  void UpdateAdBlockInstanceWithDAT(const base::FilePath& dat_location,
                                    const std::string& resources = "[]");
  void UpdateCustomAdBlockInstanceWithRules(
      const std::string& rules,
      const std::string& resources = "[]");
  void AssertTagExists(const std::string& tag, bool expected_exists) const;
  void InitEmbeddedTestServer();
  void GetTestDataDir(base::FilePath* test_data_dir);
  void SetDefaultComponentIdAndBase64PublicKeyForTest();
  void SetRegionalComponentIdAndBase64PublicKeyForTest();
  bool InstallDefaultAdBlockComponent(
      const std::string& extension_dir = "adblock-default");
  bool InstallRegionalAdBlockComponent(const std::string& uuid,
                                       bool enable_list = true);
  void SetSubscriptionIntervals();
  void WaitForAdBlockServiceThreads();
  void ShieldsDown(const GURL& url);
  void DisableAggressiveMode();
  void LoadDAT(base::FilePath path);
  void EnableRedirectUrlParsing();
  content::WebContents* web_contents();

  std::vector<std::unique_ptr<brave_shields::TestFiltersProvider>>
      source_providers_;

  net::SpawnedTestServer ws_server_;
  net::EmbeddedTestServer dynamic_server_;
  net::EmbeddedTestServer https_server_;
};

#endif  // BRAVE_BROWSER_BRAVE_SHIELDS_AD_BLOCK_SERVICE_BROWSERTEST_H_
