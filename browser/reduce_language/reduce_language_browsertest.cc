/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/scoped_observation.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/browser/extensions/brave_base_local_data_files_browsertest.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/features.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/reduce_language/browser/reduce_language_component_installer.h"
#include "brave/components/reduce_language/browser/reduce_language_service.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/language/core/browser/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/common/content_paths.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/base/features.h"
#include "url/gurl.h"

using brave_shields::ControlType;
using brave_shields::features::kBraveReduceLanguage;

namespace {

const char kTestDataDirectory[] = "reduce-language-data";

}  // namespace

namespace reduce_language {

class ReduceLanguageComponentInstallerPolicyWaiter
    : public ReduceLanguageComponentInstallerPolicy::Observer {
 public:
  explicit ReduceLanguageComponentInstallerPolicyWaiter(
      ReduceLanguageComponentInstallerPolicy* component_installer)
      : component_installer_(component_installer), scoped_observer_(this) {
    scoped_observer_.Observe(component_installer_);
  }
  ReduceLanguageComponentInstallerPolicyWaiter(
      const ReduceLanguageComponentInstallerPolicyWaiter&) = delete;
  ReduceLanguageComponentInstallerPolicyWaiter& operator=(
      const ReduceLanguageComponentInstallerPolicyWaiter&) = delete;
  ~ReduceLanguageComponentInstallerPolicyWaiter() override = default;

  void Wait() { run_loop_.Run(); }

 private:
  // ReduceLanguageComponentInstallerPolicy::Observer:
  void OnRulesReady(const std::string& json_content) override {
    run_loop_.QuitWhenIdle();
  }

  raw_ptr<ReduceLanguageComponentInstallerPolicy> const component_installer_;
  base::RunLoop run_loop_;
  base::ScopedObservation<ReduceLanguageComponentInstallerPolicy,
                          ReduceLanguageComponentInstallerPolicy::Observer>
      scoped_observer_{this};
};

class BraveReduceLanguageBrowserTest : public BaseLocalDataFilesBrowserTest {
 public:
  // BaseLocalDataFilesBrowserTest overrides
  const char* test_data_directory() override { return kTestDataDirectory; }
  const char* embedded_test_server_directory() override { return ""; }
  LocalDataFilesObserver* service() override {
    return g_brave_browser_process->reduce_language_component_installer();
  }

  BraveReduceLanguageBrowserTest() {
    feature_list_.InitAndEnableFeature(kBraveReduceLanguage);
    embedded_test_server()->RegisterRequestMonitor(
        base::BindRepeating(&BraveReduceLanguageBrowserTest::MonitorHTTPRequest,
                            base::Unretained(this)));
  }

  void WaitForService() override {
    // Wait for reduce-language component installer to load and parse its
    // configuration file.
    reduce_language::ReduceLanguageComponentInstallerPolicy*
        component_installer =
            g_brave_browser_process->reduce_language_component_installer();
    ReduceLanguageComponentInstallerPolicyWaiter(component_installer).Wait();
  }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  void BlockFingerprinting(std::string domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::BLOCK,
        embedded_test_server()->GetURL(domain, "/"));
  }

  void SetAcceptLanguages(const std::string& accept_languages) {
    content::BrowserContext* context =
        static_cast<content::BrowserContext*>(browser()->profile());
    PrefService* prefs = user_prefs::UserPrefs::Get(context);
    prefs->Set(language::prefs::kSelectedLanguages,
               base::Value(accept_languages));
  }

  void MonitorHTTPRequest(const net::test_server::HttpRequest& request) {
    if (request.relative_url.find("/reduce-language/") == std::string::npos) {
      return;
    }
    if (expected_http_accept_language_.empty()) {
      return;
    }
    ASSERT_EQ(request.headers.at("accept-language"),
              expected_http_accept_language_);
  }

  void SetExpectedHTTPAcceptLanguage(
      const std::string& expected_http_accept_language) {
    expected_http_accept_language_ = expected_http_accept_language;
  }

 private:
  base::test::ScopedFeatureList feature_list_;
  std::string expected_http_accept_language_;
};

// Tests results of farbling HTTP Accept-Language header
IN_PROC_BROWSER_TEST_F(BraveReduceLanguageBrowserTest,
                       DoNotFarbleIfDomainIsOnExceptionList) {
  std::string domain_b = "b.test";
  std::string domain_x = "www.ulta.com";
  std::string domain_y = "aeroplan.rewardops.com";
  std::string domain_z = "login.live.com";
  GURL url_b = embedded_test_server()->GetURL(
      domain_b, "/reduce-language/page-with-subresources.html");
  GURL url_x = embedded_test_server()->GetURL(
      domain_x, "/reduce-language/page-with-subresources.html");
  GURL url_y = embedded_test_server()->GetURL(
      domain_y, "/reduce-language/page-with-subresources.html");
  GURL url_z = embedded_test_server()->GetURL(
      domain_z, "/reduce-language/page-with-subresources.html");
  SetAcceptLanguages("la,es,en");

  ASSERT_TRUE(InstallMockExtension());

  // Farbling level: maximum
  // HTTP Accept-Language header should be farbled but the same across domains.
  // This is a sanity check that farbling works as expected for domains that
  // are not on the exception list.
  BlockFingerprinting(domain_b);
  SetExpectedHTTPAcceptLanguage("en-US,en;q=0.9");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b));

  // Farbling level: maximum but domain is on exceptions list
  // HTTP Accept-Language header should not be farbled.
  BlockFingerprinting(domain_x);
  SetExpectedHTTPAcceptLanguage("la,es;q=0.9,en;q=0.8");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_x));
  // BlockFingerprinting(domain_y);
  // ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_y));
  // BlockFingerprinting(domain_z);
  // ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_z));
}

}  // namespace reduce_language
