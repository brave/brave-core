/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/base64url.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/extensions/brave_base_local_data_files_browsertest.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/debounce/browser/debounce_download_service.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "net/base/url_util.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/default_handlers.h"

using debounce::DebounceDownloadService;

const char kTestDataDirectory[] = "debounce-data";

class DebounceDownloadServiceWaiter : public DebounceDownloadService::Observer {
 public:
  explicit DebounceDownloadServiceWaiter(
      DebounceDownloadService* download_service)
      : download_service_(download_service), scoped_observer_(this) {
    scoped_observer_.Add(download_service_);
  }
  ~DebounceDownloadServiceWaiter() override = default;

  void Wait() { run_loop_.Run(); }

 private:
  // DebounceDownloadService::Observer:
  void OnRulesReady(DebounceDownloadService* download_service) override {
    run_loop_.QuitWhenIdle();
  }

  DebounceDownloadService* const download_service_;
  base::RunLoop run_loop_;
  ScopedObserver<DebounceDownloadService, DebounceDownloadService::Observer>
      scoped_observer_;

  DISALLOW_COPY_AND_ASSIGN(DebounceDownloadServiceWaiter);
};

class DebounceBrowserTest : public BaseLocalDataFilesBrowserTest {
 public:
  void SetUpOnMainThread() override {
    BaseLocalDataFilesBrowserTest::SetUpOnMainThread();
  }

  // BaseLocalDataFilesBrowserTest overrides
  const char* test_data_directory() override { return kTestDataDirectory; }
  const char* embedded_test_server_directory() override { return ""; }
  LocalDataFilesObserver* service() override {
    return g_brave_browser_process->debounce_download_service();
  }

  void WaitForService() override {
    // Wait for debounce download service to load and parse its
    // configuration file.
    debounce::DebounceDownloadService* download_service =
        g_brave_browser_process->debounce_download_service();
    DebounceDownloadServiceWaiter(download_service).Wait();
  }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  GURL add_redirect_param(const GURL& original_url,
                                 const GURL& landing_url) {
    return net::AppendOrReplaceQueryParameter(original_url, "url",
                                              landing_url.spec());
  }

  GURL add_base64_redirect_param(const GURL& original_url,
                                 const GURL& landing_url) {
    std::string encoded_destination;
    base::Base64UrlEncode(landing_url.spec(),
                          base::Base64UrlEncodePolicy::OMIT_PADDING,
                          &encoded_destination);
    const std::string query =
      base::StringPrintf("url=%s", encoded_destination.c_str());
    GURL::Replacements replacement;
    replacement.SetQueryStr(query);
    return original_url.ReplaceComponents(replacement);
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void NavigateToURLAndWaitForRedirects(const GURL& original_url,
                                        const GURL& landing_url) {
    ui_test_utils::UrlLoadObserver load_complete(
        landing_url, content::NotificationService::AllSources());
    ui_test_utils::NavigateToURL(browser(), original_url);
    load_complete.Wait();
    EXPECT_EQ(contents()->GetLastCommittedURL(), landing_url);
  }
};

IN_PROC_BROWSER_TEST_F(DebounceBrowserTest,
                       Redirect) {
  ASSERT_TRUE(InstallMockExtension());
  GURL base_url = GURL("http://simple.a.com/");
  GURL landing_url = GURL("http://simple.b.com/");
  GURL original_url = add_redirect_param(base_url, landing_url);
  NavigateToURLAndWaitForRedirects(original_url, landing_url);
}

IN_PROC_BROWSER_TEST_F(DebounceBrowserTest,
                       Base64Redirect) {
  ASSERT_TRUE(InstallMockExtension());
  GURL base_url = GURL("http://base64.a.com/");
  GURL landing_url = GURL("http://base64.b.com/");
  GURL original_url = add_base64_redirect_param(base_url, landing_url);
  NavigateToURLAndWaitForRedirects(original_url, landing_url);
}
