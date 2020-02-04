/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/browser/brave_drm_tab_helper.h"
#include "brave/browser/widevine/widevine_utils.h"
#include "brave/common/brave_paths.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/permissions/permission_request_manager.h"
#include "chrome/browser/ssl/cert_verifier_browser_test.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "third_party/widevine/cdm/buildflags.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
#include <string>

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/widevine/brave_widevine_bundle_manager.h"
#endif

namespace {
class TestObserver : public PermissionRequestManager::Observer {
 public:
  void OnBubbleAdded() override {
    added_count_++;
    bubble_added_ = true;
  }
  bool bubble_added_ = false;
  int added_count_ = 0;
};
}  // namespace

class WidevinePermissionRequestBrowserTest
    : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    GetPermissionRequestManager()->AddObserver(&observer);
  }

  void TearDownOnMainThread() override {
    InProcessBrowserTest::TearDownOnMainThread();
    GetPermissionRequestManager()->RemoveObserver(&observer);
  }

  content::WebContents* GetActiveWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  PermissionRequestManager* GetPermissionRequestManager() {
    return PermissionRequestManager::FromWebContents(GetActiveWebContents());
  }

  BraveDrmTabHelper* GetBraveDrmTabHelper() {
    return BraveDrmTabHelper::FromWebContents(GetActiveWebContents());
  }

  TestObserver observer;
};

IN_PROC_BROWSER_TEST_F(WidevinePermissionRequestBrowserTest, VisibilityTest) {
  GetPermissionRequestManager()->set_auto_response_for_test(
      PermissionRequestManager::DISMISS);
  auto* drm_tab_helper = GetBraveDrmTabHelper();

  // Check permission bubble is visible.
  drm_tab_helper->OnWidevineKeySystemAccessRequest();
  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(observer.bubble_added_);

  // Check permission is not requested again for same site.
  observer.bubble_added_ = false;
  drm_tab_helper->OnWidevineKeySystemAccessRequest();
  content::RunAllTasksUntilIdle();
  EXPECT_FALSE(observer.bubble_added_);

  // Check permission is requested again after new navigation.
  observer.bubble_added_ = false;
  EXPECT_TRUE(content::NavigateToURL(GetActiveWebContents(),
                                     GURL("chrome://newtab/")));
  drm_tab_helper->OnWidevineKeySystemAccessRequest();
  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(observer.bubble_added_);

  // Check permission bubble is not visible when user turns it off.
  observer.bubble_added_ = false;
  DontAskWidevineInstall(GetActiveWebContents(), true);
  EXPECT_TRUE(content::NavigateToURL(GetActiveWebContents(),
                                     GURL("chrome://newtab/")));
  drm_tab_helper->OnWidevineKeySystemAccessRequest();
  content::RunAllTasksUntilIdle();
  EXPECT_FALSE(observer.bubble_added_);

  // Check permission bubble is visible when user turns it on.
  observer.bubble_added_ = false;
  DontAskWidevineInstall(GetActiveWebContents(), false);
  EXPECT_TRUE(content::NavigateToURL(GetActiveWebContents(),
                                     GURL("chrome://newtab/")));
  drm_tab_helper->OnWidevineKeySystemAccessRequest();
  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(observer.bubble_added_);
}

// Check extra text is added.
IN_PROC_BROWSER_TEST_F(WidevinePermissionRequestBrowserTest, BubbleTest) {
  auto* permission_request_manager =
      GetPermissionRequestManager();
  EXPECT_FALSE(permission_request_manager->IsRequestInProgress());
  GetBraveDrmTabHelper()->OnWidevineKeySystemAccessRequest();
  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(permission_request_manager->IsRequestInProgress());

  gfx::NativeWindow window =
      permission_request_manager->GetBubbleWindow();
  auto* widget = views::Widget::GetWidgetForNativeWindow(window);
  DCHECK(widget);

  auto* delegate_view =
      static_cast<views::BubbleDialogDelegateView*>(
          widget->widget_delegate());
  DCHECK(delegate_view);
  // Original PermissionsBubbleDialogDelegateView has one child.
  // It's label that includes icon and fragment test.
  // For widevine permission requests, two more child views are added.
  // one for extra label and the other one is do not ask checkbox.
  EXPECT_EQ(3ul, delegate_view->children().size());
}

// OptedInPref of bundling tests are done by
// BraveWidevineBundleManagerBrowserTest.
#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT)
IN_PROC_BROWSER_TEST_F(WidevinePermissionRequestBrowserTest,
                       CheckOptedInPrefStateForComponent) {
  // Before we allow, opted in should be false
  EXPECT_FALSE(IsWidevineOptedIn());

  GetPermissionRequestManager()->set_auto_response_for_test(
      PermissionRequestManager::ACCEPT_ALL);
  auto* drm_tab_helper = GetBraveDrmTabHelper();
  drm_tab_helper->OnWidevineKeySystemAccessRequest();
  content::RunAllTasksUntilIdle();

  // After we allow, opted in pref should be true
  EXPECT_TRUE(IsWidevineOptedIn());
  EXPECT_TRUE(observer.bubble_added_);

  // Reset observer and check permission bubble isn't created again.
  observer.bubble_added_ = false;
  drm_tab_helper->OnWidevineKeySystemAccessRequest();
  content::RunAllTasksUntilIdle();
  EXPECT_FALSE(observer.bubble_added_);
}
#endif

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
// For bundling, PermissionRequest for browser restart is added after finishing
// installis done. Check seconds permission request is also added.
IN_PROC_BROWSER_TEST_F(WidevinePermissionRequestBrowserTest,
                       TriggerTwoPermissionTest) {
  auto* bundle_manager =
      g_brave_browser_process->brave_widevine_bundle_manager();
  bundle_manager->startup_checked_ = true;
  bundle_manager->is_test_ = true;

  TestObserver observer;
  auto* permission_request_manager = GetPermissionRequestManager();
  permission_request_manager->AddObserver(&observer);
  permission_request_manager->set_auto_response_for_test(
      PermissionRequestManager::ACCEPT_ALL);

  GetBraveDrmTabHelper()->OnWidevineKeySystemAccessRequest();
  content::RunAllTasksUntilIdle();
  bundle_manager->InstallDone(std::string());
  content::RunAllTasksUntilIdle();

  // Check two permission bubble are created.
  EXPECT_EQ(2, observer.added_count_);
  permission_request_manager->RemoveObserver(&observer);
}
#endif

class ScriptTriggerWidevinePermissionRequestBrowserTest
    : public CertVerifierBrowserTest {
 public:
  ScriptTriggerWidevinePermissionRequestBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    CertVerifierBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    // Chromium allows the API under test only on HTTPS domains.
    base::FilePath test_data_dir;
    base::PathService::Get(chrome::DIR_TEST_DATA, &test_data_dir);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    SetUpMockCertVerifierForHttpsServer(0, net::OK);
    ASSERT_TRUE(https_server_.Start());

    GetPermissionRequestManager()->AddObserver(&observer);
  }

  void TearDownOnMainThread() override {
    CertVerifierBrowserTest::TearDownOnMainThread();
    GetPermissionRequestManager()->RemoveObserver(&observer);
  }

  void SetUpDefaultCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchASCII(
        "enable-blink-features",
        "EncryptedMediaEncryptionSchemeQuery");
  }

  content::WebContents* active_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  PermissionRequestManager* GetPermissionRequestManager() {
    return PermissionRequestManager::FromWebContents(active_contents());
  }

  bool IsPermissionBubbleShown() {
    return observer.bubble_added_;
  }

  void ResetBubbleState() {
    observer.bubble_added_ = false;
    observer.added_count_ = 0;
  }

 protected:
  void SetUpMockCertVerifierForHttpsServer(net::CertStatus cert_status,
                                           int net_result) {
    scoped_refptr<net::X509Certificate> cert(https_server_.GetCertificate());
    net::CertVerifyResult verify_result;
    verify_result.is_issued_by_known_root = true;
    verify_result.verified_cert = cert;
    verify_result.cert_status = cert_status;
    mock_cert_verifier()->AddResultForCert(cert, verify_result, net_result);
  }

  TestObserver observer;
  net::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(ScriptTriggerWidevinePermissionRequestBrowserTest,
                       SuggestPermissionIfWidevineDetected) {
  // In this test, we just want to know whether permission bubble is shown.
  GURL url = https_server_.GetURL("a.com", "/simple.html");
  ui_test_utils::NavigateToURL(browser(), url);
  EXPECT_FALSE(IsPermissionBubbleShown());

  const std::string drm_js =
      "var config = [{initDataTypes: ['cenc']}];"
      "navigator.requestMediaKeySystemAccess($1, config);";
  const std::string widevine_js = content::JsReplace(drm_js,
                                                     "com.widevine.alpha");

  EXPECT_TRUE(content::ExecuteScript(active_contents(), widevine_js));
  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(IsPermissionBubbleShown());
  ResetBubbleState();

  // The bubble should be disappeared after reload.
  content::TestNavigationObserver observer(active_contents());
  chrome::Reload(browser(), WindowOpenDisposition::CURRENT_TAB);
  observer.Wait();
  content::RunAllTasksUntilIdle();
  EXPECT_FALSE(IsPermissionBubbleShown());
  ResetBubbleState();

  // Navigate to a page with some videos.
  url = https_server_.GetURL("a.com", "/media/youtube.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::RunAllTasksUntilIdle();
  EXPECT_FALSE(IsPermissionBubbleShown());
  ResetBubbleState();

  // Check that non-widevine DRM is ignored.
  EXPECT_TRUE(
      content::ExecuteScript(active_contents(),
                             content::JsReplace(drm_js, "org.w3.clearkey")));
  content::RunAllTasksUntilIdle();
  EXPECT_FALSE(IsPermissionBubbleShown());
  ResetBubbleState();

  // Finally check the widevine request.
  EXPECT_TRUE(content::ExecuteScript(active_contents(), widevine_js));
  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(IsPermissionBubbleShown());
}
