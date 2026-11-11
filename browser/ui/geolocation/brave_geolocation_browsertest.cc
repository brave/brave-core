/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/path_service.h"
#include "base/test/test_future.h"
#include "brave/browser/ui/geolocation/brave_geolocation_permission_tab_helper.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/resource_coordinator/lifecycle_unit_state.mojom.h"
#include "chrome/browser/resource_coordinator/tab_lifecycle_unit_external.h"
#include "chrome/browser/resource_coordinator/tab_lifecycle_unit_source.h"
#include "chrome/browser/ssl/cert_verifier_browser_test.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/permissions/permission_request_manager.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/dns/mock_host_resolver.h"
#include "url/gurl.h"

class GeolocationPermissionRequestBrowserTest : public CertVerifierBrowserTest {
 public:
  GeolocationPermissionRequestBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    CertVerifierBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    base::FilePath test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    mock_cert_verifier()->set_default_result(net::OK);

    ASSERT_TRUE(https_server_.Start());
  }

  content::WebContents* active_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  BraveGeolocationPermissionTabHelper* active_tab_helper() {
    return BraveGeolocationPermissionTabHelper::From(
        browser()->GetActiveTabInterface());
  }

  // Requests a high accuracy position, and waits for the frame to report it.
  // Blink reports the accuracy only when the page's pending requests change it,
  // and requests never complete here, so only the first one is reported.
  void RequestHighAccuracyPosition() {
    // Auto-accept any permissions prompts
    permissions::PermissionRequestManager::FromWebContents(active_contents())
        ->set_auto_response_for_test(permissions::PermissionRequestManager::
                                         AutoResponseType::ACCEPT_ALL);
    base::test::TestFuture<bool> reported_future;
    active_tab_helper()->SetEnableHighAccuracyCallbackForTesting(
        reported_future.GetRepeatingCallback());
    ASSERT_EQ(
        base::Value(),
        content::EvalJs(active_contents(),
                        "navigator.geolocation.getCurrentPosition(() => {}, "
                        "() => {}, { enableHighAccuracy : true })"));
    EXPECT_TRUE(reported_future.Take());
    active_tab_helper()->SetEnableHighAccuracyCallbackForTesting(
        base::RepeatingCallback<void(bool)>());
  }

 protected:
  net::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(GeolocationPermissionRequestBrowserTest,
                       SetEnableHighAccuracyTest) {
  GURL url = https_server_.GetURL("a.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  auto* tab_helper = active_tab_helper();
  ASSERT_TRUE(tab_helper);
  EXPECT_FALSE(tab_helper->enable_high_accuracy());

  RequestHighAccuracyPosition();
  EXPECT_TRUE(tab_helper->enable_high_accuracy());

  // Reload clear high_accuracy bit from tab helper.
  content::TestNavigationObserver observer(active_contents());
  chrome::Reload(browser(), WindowOpenDisposition::kCurrentTab);
  observer.Wait();
  EXPECT_FALSE(tab_helper->enable_high_accuracy());
}

// Discarding a tab replaces its contents, whose frames must still reach the
// tab helper.
IN_PROC_BROWSER_TEST_F(GeolocationPermissionRequestBrowserTest,
                       SetEnableHighAccuracyAfterDiscard) {
  GURL url = https_server_.GetURL("a.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::kNewBackgroundTab,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  TabStripModel* const tab_strip_model = browser()->tab_strip_model();
  const int index = tab_strip_model->count() - 1;
  auto* const lifecycle_unit =
      resource_coordinator::TabLifecycleUnitSource::GetTabLifecycleUnitExternal(
          tab_strip_model->GetWebContentsAt(index));
  ASSERT_TRUE(lifecycle_unit);
  ASSERT_TRUE(
      lifecycle_unit->DiscardTab(::mojom::LifecycleUnitDiscardReason::URGENT));

  content::TestNavigationObserver navigation_observer(
      tab_strip_model->GetWebContentsAt(index));
  tab_strip_model->ActivateTabAt(index);
  active_contents()->GetController().LoadIfNecessary();
  navigation_observer.Wait();

  auto* tab_helper = active_tab_helper();
  ASSERT_TRUE(tab_helper);
  EXPECT_FALSE(tab_helper->enable_high_accuracy());

  RequestHighAccuracyPosition();
  EXPECT_TRUE(tab_helper->enable_high_accuracy());
}
