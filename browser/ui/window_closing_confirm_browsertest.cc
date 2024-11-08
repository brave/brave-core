/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/functional/bind.h"
#include "base/run_loop.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/window_closing_confirm_dialog_view.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/download/download_manager_utils.h"
#include "chrome/browser/download/download_prefs.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/lifetime/application_lifetime_desktop.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/webui/profile_helper.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/javascript_dialogs/app_modal_dialog_controller.h"
#include "components/javascript_dialogs/app_modal_dialog_view.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/download_manager.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/download_test_observer.h"
#include "content/public/test/test_download_http_response.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/views/widget/widget_observer.h"

namespace {

javascript_dialogs::AppModalDialogView* GetNextDialog() {
  javascript_dialogs::AppModalDialogController* dialog =
      ui_test_utils::WaitForAppModalDialog();
  CHECK(dialog->view());
  return dialog->view();
}

void AcceptClose() {
  GetNextDialog()->AcceptAppModalDialog();
}

void CancelClose() {
  GetNextDialog()->CancelAppModalDialog();
}

}  // namespace

class WindowClosingConfirmBrowserTest : public InProcessBrowserTest,
                                        public views::WidgetObserver {
 public:
  void SetUpOnMainThread() override {
    BraveBrowser::SuppressBrowserWindowClosingDialogForTesting(false);

    InProcessBrowserTest::SetUpOnMainThread();

    PrefService* prefs = browser()->profile()->GetPrefs();
    // Enabled by default.
    EXPECT_TRUE(prefs->GetBoolean(kEnableWindowClosingConfirm));

    SetDialogCreationCallback();
  }

  void TearDownOnMainThread() override {
    BraveBrowser::SuppressBrowserWindowClosingDialogForTesting(true);

    InProcessBrowserTest::TearDownOnMainThread();
  }

  void SetDialogCreationCallback() {
    WindowClosingConfirmDialogView::SetCreationCallbackForTesting(
        base::BindRepeating(&WindowClosingConfirmBrowserTest::
                                OnWindowClosingConfirmDialogCreated,
                            base::Unretained(this)));
  }

  void OnWindowClosingConfirmDialogCreated(views::DialogDelegateView* view) {
    view->GetWidget()->AddObserver(this);

    // This check detect whether multiple quit requests cause multiple dialog
    // creation.
    EXPECT_FALSE(closing_confirm_dialog_created_);

    closing_confirm_dialog_created_ = true;
    allow_to_close_ ? view->AcceptDialog() : view->CancelDialog();
  }

  void PrepareForBeforeUnloadDialog(content::WebContents* web_contents) {
    content::PrepContentsForBeforeUnloadTest(web_contents);
  }

  void PrepareForBeforeUnloadDialog(Browser* browser) {
    for (int i = 0; i < browser->tab_strip_model()->count(); i++)
      PrepareForBeforeUnloadDialog(
          browser->tab_strip_model()->GetWebContentsAt(i));
  }

  void WaitForAllBrowsersToClose() {
    while (!BrowserList::GetInstance()->empty())
      ui_test_utils::WaitForBrowserToClose();
  }

  void SetClosingBrowserCallbackAndWait() {
    run_loop_ = std::make_unique<base::RunLoop>();
    auto subscription =
        chrome::AddClosingAllBrowsersCallback(base::BindRepeating(
            &WindowClosingConfirmBrowserTest::OnClosingAllBrowserCallback,
            base::Unretained(this)));
    run_loop_->Run();
    run_loop_.reset();
  }

  // views::WidgetObserver:
  void OnWidgetDestroyed(views::Widget* widget) override {
    widget->RemoveObserver(this);

    if (run_loop_)
      run_loop_->Quit();
  }

  void WaitTillConfirmDialogClosed() {
    run_loop_ = std::make_unique<base::RunLoop>();
    run_loop_->Run();
  }

  // To detect the timing when BeforeUnloadFired() is called.
  void OnClosingAllBrowserCallback(bool closing) {
    if (run_loop_)
      run_loop_->Quit();
  }

  // Create a DownloadTestObserverInProgress that will wait for the
  // specified number of downloads to start.
  content::DownloadTestObserver* CreateInProgressWaiter(Browser* browser,
                                                        int num_downloads) {
    content::DownloadManager* download_manager =
        DownloadManagerForBrowser(browser);
    return new content::DownloadTestObserverInProgress(download_manager,
                                                       num_downloads);
  }

  content::DownloadManager* DownloadManagerForBrowser(Browser* browser) {
    return browser->profile()->GetDownloadManager();
  }

  DownloadPrefs* GetDownloadPrefs(Browser* browser) {
    return DownloadPrefs::FromDownloadManager(
        DownloadManagerForBrowser(browser));
  }

  base::FilePath GetDownloadDirectory(Browser* browser) {
    return GetDownloadPrefs(browser)->DownloadPath();
  }

  content::TestDownloadResponseHandler* test_response_handler() {
    return &test_response_handler_;
  }

  void SetDownloadConfirmReturn(bool allow) {
    BraveBrowserView::SetDownloadConfirmReturnForTesting(allow);
  }

  content::TestDownloadResponseHandler test_response_handler_;
  bool closing_confirm_dialog_created_ = false;
  bool allow_to_close_ = false;
  std::unique_ptr<base::RunLoop> run_loop_;
};

IN_PROC_BROWSER_TEST_F(WindowClosingConfirmBrowserTest, TestWithTwoNTPTabs) {
  BraveBrowser* brave_browser = static_cast<BraveBrowser*>(browser());
  // One tab. Doesn't need to ask.
  EXPECT_FALSE(brave_browser->ShouldAskForBrowserClosingBeforeHandlers());

  // Two tabs. Need to ask browser closing.
  ui_test_utils::NavigateToURLWithDisposition(
      brave_browser, GURL(url::kAboutBlankURL),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  EXPECT_TRUE(brave_browser->ShouldAskForBrowserClosingBeforeHandlers());

  closing_confirm_dialog_created_ = false;
  allow_to_close_ = true;

  // Do quit request twice and check second quit request doesn't make
  // another dialog. If it's created, DCHECK() in
  // OnWindowClosingConfirmDialogCreated() can detect.
  chrome::CloseWindow(brave_browser);
  chrome::CloseWindow(brave_browser);
  ui_test_utils::WaitForBrowserToClose(brave_browser);
  EXPECT_TRUE(closing_confirm_dialog_created_);
}

IN_PROC_BROWSER_TEST_F(WindowClosingConfirmBrowserTest, TestWithQuit) {
  BraveBrowser* brave_browser = static_cast<BraveBrowser*>(browser());
  ui_test_utils::NavigateToURLWithDisposition(
      brave_browser, GURL(url::kAboutBlankURL),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  // Should ask closing.
  EXPECT_TRUE(brave_browser->ShouldAskForBrowserClosingBeforeHandlers());

  // Should not ask for quit command.
  closing_confirm_dialog_created_ = false;
  chrome::CloseAllBrowsersAndQuit();
  WaitForAllBrowsersToClose();
  EXPECT_FALSE(closing_confirm_dialog_created_);
}

IN_PROC_BROWSER_TEST_F(WindowClosingConfirmBrowserTest,
                       TestWithProfileDeletion) {
  // Make two tabs.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(url::kAboutBlankURL),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  // Should ask closing for this browser window as this has more than one tab.
  EXPECT_TRUE(static_cast<BraveBrowser*>(browser())
                  ->ShouldAskForBrowserClosingBeforeHandlers());

  // However, should not ask for profile deletion.
  closing_confirm_dialog_created_ = false;
  webui::DeleteProfileAtPath(browser()->profile()->GetPath(),
                             ProfileMetrics::DELETE_PROFILE_SETTINGS);
  ui_test_utils::WaitForBrowserToClose(browser());
  EXPECT_FALSE(closing_confirm_dialog_created_);
}

IN_PROC_BROWSER_TEST_F(WindowClosingConfirmBrowserTest,
                       TestWithOnBeforeUnload) {
  ASSERT_TRUE(embedded_test_server()->Start());

  BraveBrowser* brave_browser = static_cast<BraveBrowser*>(browser());
  ASSERT_NO_FATAL_FAILURE(ASSERT_TRUE(ui_test_utils::NavigateToURL(
      brave_browser, embedded_test_server()->GetURL("/beforeunload.html"))));
  ui_test_utils::NavigateToURLWithDisposition(
      brave_browser, GURL(url::kAboutBlankURL),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  PrepareForBeforeUnloadDialog(brave_browser);

  // Check beforeunload dialog is launched after allowed to close window.
  allow_to_close_ = true;
  chrome::CloseWindow(brave_browser);
  EXPECT_TRUE(closing_confirm_dialog_created_);
  ASSERT_NO_FATAL_FAILURE(CancelClose());
  SetClosingBrowserCallbackAndWait();
  EXPECT_TRUE(brave_browser->ShouldAskForBrowserClosingBeforeHandlers());

  // Check window closing dialog is launched again after cancelling
  // beforeunlaod handler.
  closing_confirm_dialog_created_ = false;
  allow_to_close_ = true;
  chrome::CloseWindow(brave_browser);
  EXPECT_TRUE(closing_confirm_dialog_created_);

  // Close browser
  ASSERT_NO_FATAL_FAILURE(AcceptClose());
  ui_test_utils::WaitForBrowserToClose(brave_browser);
}

#if BUILDFLAG(IS_WIN) && defined(ADDRESS_SANITIZER)
// Upstream issue.
// Stack overflow on Win/ASan: http://crbug.com/367746304
// TODO(simonhong): Enable when master has the fix.
// https://github.com/brave/brave-browser/issues/41936
#define MAYBE_TestWithDownload DISABLED_TestWithDownload
#else
#define MAYBE_TestWithDownload TestWithDownload
#endif

IN_PROC_BROWSER_TEST_F(WindowClosingConfirmBrowserTest,
                       MAYBE_TestWithDownload) {
// On macOS, download in-progress warning is not shown for normal profile window
// closing as it can still continue after window is closed.
// However, private profile window works like normal window of other platforms.
// So, test with private profile window on macOS.
#if BUILDFLAG(IS_MAC)
  auto* brave_browser = static_cast<BraveBrowser*>(CreateIncognitoBrowser());
#else
  auto* brave_browser = static_cast<BraveBrowser*>(browser());
#endif
  brave_browser->profile()->GetPrefs()->SetBoolean(prefs::kPromptForDownload,
                                                   false);

  test_response_handler()->RegisterToTestServer(embedded_test_server());
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL url = embedded_test_server()->GetURL("/large_file");

  content::TestDownloadHttpResponse::Parameters parameters;
  parameters.size = 1024 * 1024 * 32; /* 32MB file. */
  content::TestDownloadHttpResponse::StartServing(parameters, url);

  // Ensure that we have enough disk space to download the large file.
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    int64_t free_space = base::SysInfo::AmountOfFreeDiskSpace(
        GetDownloadDirectory(brave_browser));
    ASSERT_LE(parameters.size, free_space)
        << "Not enough disk space to download. Got " << free_space;
  }

  // Make browser has two tabs.
  ui_test_utils::NavigateToURLWithDisposition(
      brave_browser, GURL(url::kAboutBlankURL),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  std::unique_ptr<content::DownloadTestObserver> progress_waiter(
      CreateInProgressWaiter(brave_browser, 1));

  // Start downloading a file, wait for it to be created.
  ui_test_utils::NavigateToURLWithDisposition(
      brave_browser, url, WindowOpenDisposition::CURRENT_TAB,
      ui_test_utils::BROWSER_TEST_NO_WAIT);
  progress_waiter->WaitForFinished();

  EXPECT_EQ(1u, progress_waiter->NumDownloadsSeenInState(
                    download::DownloadItem::IN_PROGRESS));

  // Don't allow window closing while downloading.
  allow_to_close_ = false;
  closing_confirm_dialog_created_ = false;
  SetDownloadConfirmReturn(false);
  chrome::CloseWindow(brave_browser);
  EXPECT_TRUE(closing_confirm_dialog_created_);
  EXPECT_TRUE(brave_browser->ShouldAskForBrowserClosingBeforeHandlers());
  WaitTillConfirmDialogClosed();

  // Allow window closing while downloading and don't cancel downloading.
  // Then, we could ask window closing again.
  allow_to_close_ = true;
  closing_confirm_dialog_created_ = false;
  SetDownloadConfirmReturn(false);
  chrome::CloseWindow(brave_browser);
  EXPECT_TRUE(closing_confirm_dialog_created_);
  WaitTillConfirmDialogClosed();
  SetClosingBrowserCallbackAndWait();
  EXPECT_TRUE(brave_browser->ShouldAskForBrowserClosingBeforeHandlers());

  // Close window again by cancelling download to terminate test.
  allow_to_close_ = true;
  closing_confirm_dialog_created_ = false;
  SetDownloadConfirmReturn(true);
  chrome::CloseWindow(brave_browser);
  EXPECT_TRUE(closing_confirm_dialog_created_);
}
