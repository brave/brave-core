/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_drm_tab_helper.h"
#include "brave/browser/widevine/widevine_utils.h"
#include "chrome/browser/permissions/permission_request_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/test_utils.h"
#include "third_party/widevine/cdm/buildflags.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/widget/widget.h"

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
  content::WebContents* GetActiveWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  PermissionRequestManager* GetPermissionRequestManager() {
    return PermissionRequestManager::FromWebContents(GetActiveWebContents());
  }

  BraveDrmTabHelper* GetBraveDrmTabHelper() {
    return BraveDrmTabHelper::FromWebContents(GetActiveWebContents());
  }
};

IN_PROC_BROWSER_TEST_F(WidevinePermissionRequestBrowserTest, VisibilityTest) {
  auto* permission_request_manager = GetPermissionRequestManager();
  TestObserver observer;
  permission_request_manager->AddObserver(&observer);

  permission_request_manager->set_auto_response_for_test(
      PermissionRequestManager::DISMISS);
  auto* drm_tab_helper = GetBraveDrmTabHelper();

  // Check permission bubble is visible.
  drm_tab_helper->OnWidevineKeySystemAccessRequest();
  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(observer.bubble_added_);

  // Check permission bubble is visible only once during the
  // lifetime webcontents.
  observer.bubble_added_ = false;
  drm_tab_helper->OnWidevineKeySystemAccessRequest();
  content::RunAllTasksUntilIdle();
  EXPECT_FALSE(observer.bubble_added_);
  permission_request_manager->RemoveObserver(&observer);
}

// Check extra text is added.
IN_PROC_BROWSER_TEST_F(WidevinePermissionRequestBrowserTest, BubbleTest) {
  auto* permission_request_manager =
      GetPermissionRequestManager();
  EXPECT_FALSE(permission_request_manager->IsBubbleVisible());
  GetBraveDrmTabHelper()->OnWidevineKeySystemAccessRequest();
  content::RunAllTasksUntilIdle();
  EXPECT_TRUE(permission_request_manager->IsBubbleVisible());

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
  // For widevine permission requests, one more label is added.
  EXPECT_EQ(2, delegate_view->child_count());
}

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
