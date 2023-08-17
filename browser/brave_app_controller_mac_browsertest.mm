/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#import <objc/runtime.h>
#include <stddef.h>

#include <string>

#include "base/apple/scoped_objc_class_swizzler.h"
#include "base/mac/foundation_util.h"
#include "base/test/scoped_feature_list.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/brave_app_controller_mac.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/cocoa/bookmarks/bookmark_menu_bridge.h"
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/test/bookmark_test_helpers.h"
#include "components/omnibox/browser/omnibox_view.h"
#include "content/public/test/browser_test.h"

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

namespace {

const char kTestingPage[] = "/empty.html";

class BraveAppControllerBrowserTest : public InProcessBrowserTest {
 public:
  BraveAppControllerBrowserTest() {
    features_.InitWithFeatureState(features::kBraveCopyCleanLinkByDefault,
                                   true);
  }

  BookmarkModel* WaitForBookmarkModel(Profile* profile) {
    BookmarkModel* bookmark_model =
        BookmarkModelFactory::GetForBrowserContext(profile);
    bookmarks::test::WaitForBookmarkModelToLoad(bookmark_model);
    return bookmark_model;
  }

 private:
  base::test::ScopedFeatureList features_;
};

class BraveAppControllerCleanLinkFeatureDisabledBrowserTest
    : public InProcessBrowserTest {
 public:
  BraveAppControllerCleanLinkFeatureDisabledBrowserTest() {
    features_.InitWithFeatureState(features::kBraveCopyCleanLinkByDefault,
                                   false);
  }

 private:
  base::test::ScopedFeatureList features_;
};

IN_PROC_BROWSER_TEST_F(BraveAppControllerCleanLinkFeatureDisabledBrowserTest,
                       CopyLinkItemVisible) {
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL url = embedded_test_server()->GetURL(kTestingPage);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_EQ(1u, chrome::GetTotalBrowserCount());

  BraveBrowserView* browser_view = static_cast<BraveBrowserView*>(
      BraveBrowserView::GetBrowserViewForBrowser(browser()));
  OmniboxView* omnibox_view = browser_view->GetLocationBar()->GetOmniboxView();
  omnibox_view->SetFocus(true);
  omnibox_view->SelectAll(false);
  EXPECT_TRUE(omnibox_view->IsSelectAll());
  EXPECT_TRUE(BraveBrowserWindow::From(browser()->window())->HasSelectedURL());

  BraveAppController* ac = base::mac::ObjCCastStrict<BraveAppController>(
      [[NSApplication sharedApplication] delegate]);
  ASSERT_TRUE(ac);
  NSMenu* edit_submenu = [[[NSApp mainMenu] itemWithTag:IDC_EDIT_MENU] submenu];
  NSMenuItem* copy_item = [edit_submenu itemWithTag:IDC_CONTENT_CONTEXT_COPY];
  NSMenuItem* clean_link_menu_item =
      [edit_submenu itemWithTag:IDC_COPY_CLEAN_LINK];

  [ac menuNeedsUpdate:[clean_link_menu_item menu]];
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE([clean_link_menu_item isHidden]);

  EXPECT_TRUE([[clean_link_menu_item keyEquivalent] isEqualToString:@""]);

  EXPECT_TRUE([[copy_item keyEquivalent] isEqualToString:@"c"]);
  EXPECT_EQ([copy_item keyEquivalentModifierMask], NSEventModifierFlagCommand);
}

IN_PROC_BROWSER_TEST_F(BraveAppControllerBrowserTest, CopyLinkItemVisible) {
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL url = embedded_test_server()->GetURL(kTestingPage);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_EQ(1u, chrome::GetTotalBrowserCount());

  BraveBrowserView* browser_view = static_cast<BraveBrowserView*>(
      BraveBrowserView::GetBrowserViewForBrowser(browser()));
  OmniboxView* omnibox_view = browser_view->GetLocationBar()->GetOmniboxView();
  omnibox_view->SetFocus(true);
  omnibox_view->SelectAll(false);
  EXPECT_TRUE(omnibox_view->IsSelectAll());
  EXPECT_TRUE(BraveBrowserWindow::From(browser()->window())->HasSelectedURL());

  BraveAppController* ac = base::mac::ObjCCastStrict<BraveAppController>(
      [[NSApplication sharedApplication] delegate]);
  ASSERT_TRUE(ac);

  NSMenu* edit_submenu = [[[NSApp mainMenu] itemWithTag:IDC_EDIT_MENU] submenu];
  NSMenuItem* copy_item = [edit_submenu itemWithTag:IDC_CONTENT_CONTEXT_COPY];
  NSMenuItem* clean_link_menu_item =
      [edit_submenu itemWithTag:IDC_COPY_CLEAN_LINK];

  [ac menuNeedsUpdate:[clean_link_menu_item menu]];
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE([clean_link_menu_item isHidden]);

  EXPECT_TRUE([[clean_link_menu_item keyEquivalent] isEqualToString:@"c"]);
  EXPECT_EQ([clean_link_menu_item keyEquivalentModifierMask],
            NSEventModifierFlagCommand);

  EXPECT_TRUE([[copy_item keyEquivalent] isEqualToString:@""]);
  EXPECT_EQ([copy_item keyEquivalentModifierMask], 0UL);
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

  NSMenu* edit_submenu = [[[NSApp mainMenu] itemWithTag:IDC_EDIT_MENU] submenu];
  NSMenuItem* copy_item = [edit_submenu itemWithTag:IDC_CONTENT_CONTEXT_COPY];
  NSMenuItem* clean_link_menu_item =
      [edit_submenu itemWithTag:IDC_COPY_CLEAN_LINK];

  [ac menuNeedsUpdate:[clean_link_menu_item menu]];

  EXPECT_TRUE([clean_link_menu_item isHidden]);

  EXPECT_TRUE([[clean_link_menu_item keyEquivalent] isEqualToString:@""]);
  EXPECT_EQ([clean_link_menu_item keyEquivalentModifierMask], 0UL);

  EXPECT_TRUE([[copy_item keyEquivalent] isEqualToString:@"c"]);
  EXPECT_EQ([copy_item keyEquivalentModifierMask], NSEventModifierFlagCommand);
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

  NSMenu* edit_submenu = [[[NSApp mainMenu] itemWithTag:IDC_EDIT_MENU] submenu];
  NSMenuItem* clean_link_menu_item =
      [edit_submenu itemWithTag:IDC_COPY_CLEAN_LINK];

  [ac menuNeedsUpdate:[clean_link_menu_item menu]];
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE([clean_link_menu_item isHidden]);
}

IN_PROC_BROWSER_TEST_F(BraveAppControllerBrowserTest,
                       BookmarkItemsFromMenuBarTest) {
  AppController* ac =
      base::mac::ObjCCastStrict<AppController>([NSApp delegate]);
  [ac mainMenuCreated];
  [ac setLastProfile:browser()->profile()];

  // Added one bookmark item.
  constexpr char kPersistBookmarkURL[] = "http://www.cnn.com/";
  constexpr char16_t kPersistBookmarkTitle[] = u"CNN";
  BookmarkModel* bookmark_model = WaitForBookmarkModel(browser()->profile());
  bookmarks::AddIfNotBookmarked(bookmark_model, GURL(kPersistBookmarkURL),
                                kPersistBookmarkTitle);

  // Update menubar's bookmark menu to make it includes above item.
  NSMenu* normal_window_submenu = [ac bookmarkMenuBridge]->BookmarkMenu();
  [[normal_window_submenu delegate] menuNeedsUpdate:normal_window_submenu];

  // Total 5 items - basic 3 items(Bookmark Manager, Bookmark This Tab... and
  // Bookmark All Tabs..), separator and bookmark item. and check last item is
  // bookmark item.
  EXPECT_EQ(5, [normal_window_submenu numberOfItems]);
  EXPECT_EQ(
      std::u16string(kPersistBookmarkTitle),
      base::SysNSStringToUTF16([[normal_window_submenu itemAtIndex:4] title]));

  // Create private browser and check bookmark menubar has same items.
  auto* private_browser = CreateIncognitoBrowser(browser()->profile());
  [ac setLastProfile:private_browser->profile()];
  NSMenu* private_browser_submenu = [ac bookmarkMenuBridge]->BookmarkMenu();
  [[private_browser_submenu delegate] menuNeedsUpdate:private_browser_submenu];
  EXPECT_EQ(5, [private_browser_submenu numberOfItems]);
  EXPECT_EQ(std::u16string(kPersistBookmarkTitle),
            base::SysNSStringToUTF16(
                [[private_browser_submenu itemAtIndex:4] title]));

  // Close private browser and check bookmark menubar still has same items.
  chrome::CloseWindow(private_browser);
  ui_test_utils::WaitForBrowserToClose();

  [ac setLastProfile:browser()->profile()];
  [[normal_window_submenu delegate] menuNeedsUpdate:normal_window_submenu];
  EXPECT_EQ(5, [normal_window_submenu numberOfItems]);
  EXPECT_EQ(
      std::u16string(kPersistBookmarkTitle),
      base::SysNSStringToUTF16([[normal_window_submenu itemAtIndex:4] title]));
}

}  // namespace
