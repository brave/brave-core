/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/views/omnibox/brave_omnibox_view_views.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/omnibox/omnibox_view_views.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "url/gurl.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/view_observer.h"

class BraveOmniboxViewViewsTest : public InProcessBrowserTest,
                                  public views::ViewObserver {
 public:
  LocationBarView* location_bar(Browser* browser) {
    auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
    return browser_view->toolbar()->location_bar();
  }
  OmniboxViewViews* omnibox_view(Browser* browser) {
    return location_bar(browser)->omnibox_view();
  }

  // views::ViewObserver overrides:
  void OnViewFocused(views::View* observed_view) override {
    run_loop_->Quit();
  }

  base::RunLoop* run_loop_ = nullptr;
};

// Load brave url and check copied url also has brave scheme.
IN_PROC_BROWSER_TEST_F(BraveOmniboxViewViewsTest, CopyURLToClipboardTest) {
  const std::string test_url("brave://version/");
  ui_test_utils::NavigateToURL(browser(), GURL(test_url));

  omnibox_view(browser())->SelectAll(true);
  omnibox_view(browser())->ExecuteCommand(views::Textfield::kCopy, 0);
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  std::string text_from_clipboard;
  clipboard->ReadAsciiText(ui::ClipboardBuffer::kCopyPaste,
                           &text_from_clipboard);
  EXPECT_EQ(test_url, text_from_clipboard);

#if defined(OS_LINUX)
  clipboard->ReadAsciiText(ui::ClipboardBuffer::kSelection,
                           &text_from_clipboard);
  EXPECT_EQ(test_url, text_from_clipboard);
#endif
}

IN_PROC_BROWSER_TEST_F(BraveOmniboxViewViewsTest, WaitForTorProcess) {
  auto* browser_list = BrowserList::GetInstance();
  content::WindowedNotificationObserver tor_browser_creation_observer(
      chrome::NOTIFICATION_BROWSER_OPENED,
      content::NotificationService::AllSources());
  brave::NewOffTheRecordWindowTor(browser());
  tor_browser_creation_observer.Wait();
  Browser* tor_browser = nullptr;
  for (Browser* browser : *browser_list) {
    if (brave::IsTorProfile(browser->profile())) {
      tor_browser = browser;
      break;
    }
  }
  DCHECK(tor_browser);
  OmniboxViewViews* tor_views = omnibox_view(tor_browser);
  chrome::FocusLocationBar(tor_browser);

  tor_views->AddObserver(this);

  base::RunLoop run_loop;
  if (!tor_views->HasFocus()) {
    // Wait for omnibox get focused.
    run_loop_ = &run_loop;
    run_loop.Run();
  }

  // Check indicator
  EXPECT_EQ(tor_views->GetPlaceholderText(),
            l10n_util::GetStringUTF16(IDS_OMNIBOX_INITIALIZING_TOR));
  EXPECT_EQ(tor_views->GetText(),
            l10n_util::GetStringUTF16(IDS_OMNIBOX_INITIALIZING_TOR));

  // Check read only
  tor_views->SelectAll(true);
  tor_views->ExecuteCommand(views::Textfield::kCut, 0);
  EXPECT_EQ(tor_views->GetText(),
            l10n_util::GetStringUTF16(IDS_OMNIBOX_INITIALIZING_TOR));

  // Emulate tor process launched
  static_cast<BraveOmniboxViewViews*>(tor_views)->OnTorLaunched(true, 5566);
  EXPECT_NE(tor_views->GetPlaceholderText(),
            l10n_util::GetStringUTF16(IDS_OMNIBOX_INITIALIZING_TOR));
  EXPECT_NE(tor_views->GetText(),
            l10n_util::GetStringUTF16(IDS_OMNIBOX_INITIALIZING_TOR));

  tor_views->RemoveObserver(this);
}
