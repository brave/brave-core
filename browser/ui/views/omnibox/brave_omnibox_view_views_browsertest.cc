/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/omnibox/omnibox_view_views.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "url/gurl.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/controls/textfield/textfield.h"

class BraveOmniboxViewViewsTest : public InProcessBrowserTest {
 public:
  LocationBarView* location_bar() {
    auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
    return browser_view->toolbar()->location_bar();
  }
  OmniboxViewViews* omnibox_view() { return location_bar()->omnibox_view(); }
};

// Load brave url and check copied url also has brave scheme.
IN_PROC_BROWSER_TEST_F(BraveOmniboxViewViewsTest, CopyURLToClipboardTest) {
  const std::string test_url("brave://version/");
  ui_test_utils::NavigateToURL(browser(), GURL(test_url));

  omnibox_view()->SelectAll(true);
  omnibox_view()->ExecuteCommand(views::Textfield::kCopy, 0);
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  std::string text_from_clipboard;
  clipboard->ReadAsciiText(ui::ClipboardBuffer::kCopyPaste,
                           /* data_dst = */ nullptr,
                           &text_from_clipboard);
  EXPECT_EQ(test_url, text_from_clipboard);

#if defined(OS_LINUX)
  clipboard->ReadAsciiText(ui::ClipboardBuffer::kSelection,
                           /* data_dst = */ nullptr,
                           &text_from_clipboard);
  EXPECT_EQ(test_url, text_from_clipboard);
#endif
}
