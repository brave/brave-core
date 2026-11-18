/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/speedreader/reader_mode_toolbars.h"

#include "base/check_deref.h"
#include "brave/browser/ui/speedreader/speedreader_tab_helper.h"
#include "brave/browser/ui/views/frame/split_view/brave_contents_container_view.h"
#include "brave/browser/ui/views/speedreader/reader_mode_toolbar_view.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/contents_web_view.h"
#include "chrome/browser/ui/views/frame/multi_contents_view.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"

namespace speedreader {

namespace {

bool IsTabDistilled(content::WebContents* web_contents) {
  if (!web_contents) {
    return false;
  }
  auto* tab_helper = SpeedreaderTabHelper::From(
      tabs::TabInterface::MaybeGetFromContents(web_contents));
  return tab_helper && IsDistilled(tab_helper->PageDistillState());
}

}  // namespace

void UpdateReaderModeToolbars(BrowserWindowInterface& browser) {
  // WebUI browser windows have no reader mode toolbars.
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(&browser);
  if (!browser_view) {
    return;
  }

  MultiContentsView& multi_contents_view =
      CHECK_DEREF(browser_view->multi_contents_view());
  BraveContentsContainerView::From(
      multi_contents_view.GetActiveContentsContainerView())
      ->reader_mode_toolbar()
      ->SetVisible(
          IsTabDistilled(browser.GetTabStripModel()->GetActiveWebContents()));

  // The inactive pane of a split view shows its toolbar too.
  auto* inactive_container = BraveContentsContainerView::From(
      multi_contents_view.GetInactiveContentsContainerView());
  inactive_container->reader_mode_toolbar()->SetVisible(
      IsTabDistilled(inactive_container->contents_view()->web_contents()));
}

}  // namespace speedreader
