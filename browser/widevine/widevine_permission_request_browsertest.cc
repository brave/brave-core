/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/widevine/widevine_permission_request.h"

#include <memory>

#include "base/path_service.h"
#include "brave/browser/brave_drm_tab_helper.h"
#include "brave/browser/widevine/widevine_utils.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/permissions/permission_widevine_utils.h"
#include "brave/components/widevine/constants.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ssl/cert_verifier_browser_test.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "chrome/test/permissions/permission_request_manager_test_api.h"
#include "components/prefs/pref_service.h"
#include "components/update_client/crx_update_item.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

#if BUILDFLAG(IS_LINUX)
#include "components/component_updater/component_updater_service.h"
#endif

namespace {
class TestObserver : public permissions::PermissionRequestManager::Observer {
 public:
  void OnPromptAdded() override {
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
    test_api_ =
        std::make_unique<test::PermissionRequestManagerTestApi>(browser());
    EXPECT_TRUE(test_api_->manager());
  }

  void TearDownOnMainThread() override {
    InProcessBrowserTest::TearDownOnMainThread();
    GetPermissionRequestManager()->RemoveObserver(&observer);
  }

  content::WebContents* GetActiveWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  permissions::PermissionRequestManager* GetPermissionRequestManager() {
    return permissions::PermissionRequestManager::FromWebContents(
        GetActiveWebContents());
  }

  BraveDrmTabHelper* GetBraveDrmTabHelper() {
    return BraveDrmTabHelper::FromWebContents(GetActiveWebContents());
  }

  TestObserver observer;
  std::unique_ptr<test::PermissionRequestManagerTestApi> test_api_;
};

IN_PROC_BROWSER_TEST_F(WidevinePermissionRequestBrowserTest, VisibilityTest) {
  GetPermissionRequestManager()->set_auto_response_for_test(
      permissions::PermissionRequestManager::DISMISS);
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
  static_cast<Profile*>(GetActiveWebContents()->GetBrowserContext())
      ->GetPrefs()
      ->SetBoolean(kAskEnableWidvine, false);
  EXPECT_TRUE(content::NavigateToURL(GetActiveWebContents(),
                                     GURL("chrome://newtab/")));
  drm_tab_helper->OnWidevineKeySystemAccessRequest();
  content::RunAllTasksUntilIdle();
  EXPECT_FALSE(observer.bubble_added_);

  // Check permission bubble is visible when user turns it on.
  observer.bubble_added_ = false;
  static_cast<Profile*>(GetActiveWebContents()->GetBrowserContext())
      ->GetPrefs()
      ->SetBoolean(kAskEnableWidvine, true);
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

  views::Widget* widget = test_api_->GetPromptWindow();
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

IN_PROC_BROWSER_TEST_F(WidevinePermissionRequestBrowserTest,
                       CheckOptedInPrefStateForComponent) {
  // Before we allow, opted in should be false
  EXPECT_FALSE(IsWidevineEnabled());

  GetPermissionRequestManager()->set_auto_response_for_test(
      permissions::PermissionRequestManager::ACCEPT_ALL);
  auto* drm_tab_helper = GetBraveDrmTabHelper();
  drm_tab_helper->OnWidevineKeySystemAccessRequest();
  content::RunAllTasksUntilIdle();

  // After we allow, opted in pref should be true
  EXPECT_TRUE(IsWidevineEnabled());
  EXPECT_TRUE(observer.bubble_added_);

  // Reset observer and check permission bubble isn't created again.
  observer.bubble_added_ = false;
  drm_tab_helper->OnWidevineKeySystemAccessRequest();
  content::RunAllTasksUntilIdle();
  EXPECT_FALSE(observer.bubble_added_);
}

#if BUILDFLAG(IS_LINUX)
// On linux, additional permission request is used to ask restarting.
IN_PROC_BROWSER_TEST_F(WidevinePermissionRequestBrowserTest,
                       TriggerTwoPermissionTest) {
  TestObserver observer;
  auto* permission_request_manager = GetPermissionRequestManager();
  permission_request_manager->AddObserver(&observer);
  permission_request_manager->set_auto_response_for_test(
      permissions::PermissionRequestManager::ACCEPT_ALL);

  GetBraveDrmTabHelper()->OnWidevineKeySystemAccessRequest();
  content::RunAllTasksUntilIdle();

  WidevinePermissionRequest::is_test_ = true;
  update_client::CrxUpdateItem item;
  item.id = kWidevineComponentId;
  item.state = update_client::ComponentState::kUpdated;
  GetBraveDrmTabHelper()->OnEvent(item);
  content::RunAllTasksUntilIdle();

  // Check two permission bubble are created.
  EXPECT_EQ(2, observer.added_count_);
  permission_request_manager->RemoveObserver(&observer);
}
#endif  // OS_LINUX

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
    mock_cert_verifier()->set_default_result(net::OK);

    ASSERT_TRUE(https_server_.Start());

    GetPermissionRequestManager()->AddObserver(&observer);
  }

  void TearDownOnMainThread() override {
    CertVerifierBrowserTest::TearDownOnMainThread();
    GetPermissionRequestManager()->RemoveObserver(&observer);
  }

  content::WebContents* active_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  permissions::PermissionRequestManager* GetPermissionRequestManager() {
    return permissions::PermissionRequestManager::FromWebContents(
        active_contents());
  }

  bool IsPermissionBubbleShown() {
    return observer.bubble_added_;
  }

  void ResetBubbleState() {
    observer.bubble_added_ = false;
    observer.added_count_ = 0;
  }

 protected:
  TestObserver observer;
  net::EmbeddedTestServer https_server_;
};

#if defined(OFFICIAL_BUILD) && BUILDFLAG(IS_WIN)
#define MAYBE_SuggestPermissionIfWidevineDetected \
  DISABLED_SuggestPermissionIfWidevineDetected
#else
#define MAYBE_SuggestPermissionIfWidevineDetected \
  SuggestPermissionIfWidevineDetected
#endif
IN_PROC_BROWSER_TEST_F(ScriptTriggerWidevinePermissionRequestBrowserTest,
                       MAYBE_SuggestPermissionIfWidevineDetected) {
  // In this test, we just want to know whether permission bubble is shown.
  GURL url = https_server_.GetURL("a.com", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_FALSE(IsPermissionBubbleShown());

  const std::string js_error =
      "a JavaScript error: \"NotSupportedError: Unsupported keySystem or "
      "supportedConfigurations.\"\n";

  const std::string drm_js =
      "var config = [{initDataTypes: ['cenc']}];"
      "navigator.requestMediaKeySystemAccess($1, config);";
  const std::string widevine_js = content::JsReplace(drm_js,
                                                     "com.widevine.alpha");

  EXPECT_EQ(js_error, content::EvalJs(active_contents(), widevine_js).error);
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
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::RunAllTasksUntilIdle();
  EXPECT_FALSE(IsPermissionBubbleShown());
  ResetBubbleState();

  // Check that non-widevine DRM is ignored.
  EXPECT_EQ(js_error,
            content::EvalJs(active_contents(),
                            content::JsReplace(drm_js, "org.w3.clearkey"))
                .error);
  content::RunAllTasksUntilIdle();
  EXPECT_FALSE(IsPermissionBubbleShown());
  ResetBubbleState();

  // Finally check the widevine request.
  EXPECT_EQ(js_error, content::EvalJs(active_contents(), widevine_js).error);
  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(IsPermissionBubbleShown());
}
