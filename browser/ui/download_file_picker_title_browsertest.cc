/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/run_loop.h"
#include "brave/browser/ui/brave_file_select_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/download/public/common/download_item.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/download_test_observer.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/shell_dialogs/fake_select_file_dialog.h"
#include "ui/shell_dialogs/select_file_dialog.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

class DownloadFilePickerTitleBrowserTest : public PlatformBrowserTest {
 public:
  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    embedded_test_server()->ServeFilesFromSourceDirectory("chrome/test/data");
    ASSERT_TRUE(embedded_test_server()->Start());
    chrome_test_utils::GetProfile(this)->GetPrefs()->SetBoolean(
        prefs::kPromptForDownload, true);
  }

  void TearDownOnMainThread() override {
    ui::SelectFileDialog::SetFactory(nullptr);
    PlatformBrowserTest::TearDownOnMainThread();
  }

  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }
};

IN_PROC_BROWSER_TEST_F(DownloadFilePickerTitleBrowserTest,
                       SaveDialogNamesTheDownloadingSite) {
  const GURL page_url = embedded_test_server()->GetURL("a.com", "/title1.html");
  ASSERT_TRUE(content::NavigateToURL(web_contents(), page_url));

  content::DownloadTestObserverTerminal download_observer(
      chrome_test_utils::GetProfile(this)->GetDownloadManager(), 1,
      content::DownloadTestObserver::ON_DANGEROUS_DOWNLOAD_FAIL);
  ui::FakeSelectFileDialog::Factory* factory =
      ui::FakeSelectFileDialog::RegisterFactory();
  base::RunLoop run_loop;
  factory->SetOpenCallback(run_loop.QuitClosure());
  ASSERT_TRUE(content::ExecJs(web_contents(), R"(
    const link = document.createElement('a');
    link.href = '/downloads/a_zip_file.zip';
    link.download = '';
    document.body.appendChild(link);
    link.click();
  )"));
  run_loop.Run();

  ui::FakeSelectFileDialog* dialog = factory->GetLastDialog();
  ASSERT_TRUE(dialog);
  // The download's caller is its own same-origin URL.
  const url::Origin page_origin = url::Origin::Create(page_url);
  const std::u16string expected_title =
      brave::GetFileSelectTitle(web_contents(), page_origin, page_origin,
                                brave::FileSelectTitleType::kSave);
  EXPECT_FALSE(expected_title.empty());
  EXPECT_EQ(expected_title, dialog->title());
  dialog->CallFileSelectionCanceled();
  download_observer.WaitForFinished();
  EXPECT_EQ(1u, download_observer.NumDownloadsSeenInState(
                    download::DownloadItem::CANCELLED));
}

}  // namespace
