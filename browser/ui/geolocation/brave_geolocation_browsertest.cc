/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/path_service.h"
#include "brave/browser/ui/geolocation/brave_geolocation_permission_tab_helper.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/ssl/cert_verifier_browser_test.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "url/gurl.h"

class GeolocationPermissionRequestBrowserTest : public CertVerifierBrowserTest {
 public:
  GeolocationPermissionRequestBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    CertVerifierBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    brave::RegisterPathProvider();
    base::FilePath test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    mock_cert_verifier()->set_default_result(net::OK);

    ASSERT_TRUE(https_server_.Start());
  }

  content::WebContents* active_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

 protected:
  net::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(GeolocationPermissionRequestBrowserTest,
                       SetEnableHighAccuracyTest) {
  GURL url = https_server_.GetURL("a.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  auto* tab_helper =
      BraveGeolocationPermissionTabHelper::FromWebContents(active_contents());
  EXPECT_FALSE(tab_helper->enable_high_accuracy());

  const std::string get_current_position_js_with_high =
      "navigator.geolocation.getCurrentPosition(() => {}, () => {}, { "
      "enableHighAccuracy : true })";
  ASSERT_EQ(nullptr, content::EvalJs(active_contents(),
                                     get_current_position_js_with_high));
  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(tab_helper->enable_high_accuracy());

  // Reload clear high_accuracy bit from tab helper.
  content::TestNavigationObserver observer(active_contents());
  chrome::Reload(browser(), WindowOpenDisposition::CURRENT_TAB);
  observer.Wait();
  EXPECT_FALSE(tab_helper->enable_high_accuracy());

  const std::string get_current_position_js_without_high =
      "navigator.geolocation.getCurrentPosition(() => {}, () => {}, { "
      "enableHighAccuracy : false })";
  ASSERT_EQ(nullptr, content::EvalJs(active_contents(),
                                     get_current_position_js_without_high));
  content::RunAllTasksUntilIdle();
  EXPECT_FALSE(tab_helper->enable_high_accuracy());
}
