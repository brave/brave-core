/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#import <objc/runtime.h>
#include <stddef.h>

#include <string>

#include "base/mac/foundation_util.h"
#include "base/mac/scoped_nsobject.h"
#include "base/mac/scoped_objc_class_swizzler.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/brave_app_controller_mac.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/omnibox/browser/omnibox_view.h"
#include "content/public/test/browser_test.h"

namespace {

const char kTestingPage[] = "/empty.html";

using BraveAppControllerBrowserTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(BraveAppControllerBrowserTest, CopyLinkItemVisible) {
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL url = embedded_test_server()->GetURL(kTestingPage);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_EQ(1u, chrome::GetTotalBrowserCount());

  BraveBrowserView* browser_view = static_cast<BraveBrowserView*>(
      BraveBrowserView::GetBrowserViewForBrowser(browser()));
  OmniboxView* omnibox_view = browser_view->GetLocationBar()->GetOmniboxView();
  omnibox_view->SelectAll(false);
  EXPECT_TRUE(omnibox_view->IsSelectAll());
  EXPECT_TRUE(BraveBrowserWindow::From(browser()->window())->HasSelectedURL());

  BraveAppController* ac = base::mac::ObjCCastStrict<BraveAppController>(
      [[NSApplication sharedApplication] delegate]);
  ASSERT_TRUE(ac);
  base::scoped_nsobject<NSMenu> edit_submenu(
      [[[NSApp mainMenu] itemWithTag:IDC_EDIT_MENU] submenu],
      base::scoped_policy::RETAIN);

  base::scoped_nsobject<NSMenuItem> clean_link_menu_item(
      [edit_submenu itemWithTag:IDC_COPY_CLEAN_LINK],
      base::scoped_policy::RETAIN);

  [ac menuNeedsUpdate:[clean_link_menu_item menu]];
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE([clean_link_menu_item isHidden]);
}

IN_PROC_BROWSER_TEST_F(BraveAppControllerBrowserTest, CopyLinkItemNotVisible) {
  EXPECT_EQ(1u, chrome::GetTotalBrowserCount());
  OmniboxView* omnibox_view =
      browser()->window()->GetLocationBar()->GetOmniboxView();
  omnibox_view->SetUserText(u"any text");
  omnibox_view->SelectAll(false);
  EXPECT_TRUE(omnibox_view->IsSelectAll());
  AppController* ac = base::mac::ObjCCastStrict<AppController>(
      [[NSApplication sharedApplication] delegate]);
  ASSERT_TRUE(ac);
  base::scoped_nsobject<NSMenu> edit_submenu(
      [[[NSApp mainMenu] itemWithTag:IDC_EDIT_MENU] submenu],
      base::scoped_policy::RETAIN);

  base::scoped_nsobject<NSMenuItem> clean_link_menu_item(
      [edit_submenu itemWithTag:IDC_COPY_CLEAN_LINK],
      base::scoped_policy::RETAIN);

  [ac menuNeedsUpdate:[clean_link_menu_item menu]];

  EXPECT_TRUE([clean_link_menu_item isHidden]);
}

IN_PROC_BROWSER_TEST_F(BraveAppControllerBrowserTest,
                       CopyLinkItemNotVisibleWithoutSelection) {
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL url = embedded_test_server()->GetURL(kTestingPage);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_EQ(1u, chrome::GetTotalBrowserCount());

  BraveBrowserView* browser_view = static_cast<BraveBrowserView*>(
      BraveBrowserView::GetBrowserViewForBrowser(browser()));
  OmniboxView* omnibox_view = browser_view->GetLocationBar()->GetOmniboxView();
  EXPECT_FALSE(omnibox_view->IsSelectAll());
  EXPECT_FALSE(BraveBrowserWindow::From(browser()->window())->HasSelectedURL());

  BraveAppController* ac = base::mac::ObjCCastStrict<BraveAppController>(
      [[NSApplication sharedApplication] delegate]);
  ASSERT_TRUE(ac);
  base::scoped_nsobject<NSMenu> edit_submenu(
      [[[NSApp mainMenu] itemWithTag:IDC_EDIT_MENU] submenu],
      base::scoped_policy::RETAIN);

  base::scoped_nsobject<NSMenuItem> clean_link_menu_item(
      [edit_submenu itemWithTag:IDC_COPY_CLEAN_LINK],
      base::scoped_policy::RETAIN);

  [ac menuNeedsUpdate:[clean_link_menu_item menu]];
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE([clean_link_menu_item isHidden]);
}

}  // namespace
