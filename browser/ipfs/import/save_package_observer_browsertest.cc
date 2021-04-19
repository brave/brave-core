/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/import/save_package_observer.h"

#include "base/files/file_util.h"
#include "chrome/browser/download/download_item_model.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/download/public/common/download_item.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/download_manager.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/fake_download_item.h"
#include "net/base/filename_util.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

class SavePackageFinishedObserverBrowserTest : public InProcessBrowserTest {
 public:
  SavePackageFinishedObserverBrowserTest() {}
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    ASSERT_TRUE(embedded_test_server()->Start());
    ASSERT_TRUE(save_dir_.CreateUniqueTempDir());
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void GetDestinationPaths(const std::string& prefix,
                           base::FilePath* full_file_name,
                           base::FilePath* dir) {
    *full_file_name = save_dir_.GetPath().AppendASCII(prefix + ".html");
    *dir = save_dir_.GetPath().AppendASCII(prefix + "_files");
  }

 private:
  base::ScopedTempDir save_dir_;
};

class FakePackageDownloadItem : public content::FakeDownloadItem {
 public:
  FakePackageDownloadItem() {}
  ~FakePackageDownloadItem() override {}

  download::DownloadItem::DownloadCreationType GetDownloadCreationType()
      const override {
    return type_;
  }

  void SetDownloadCreationType(DownloadCreationType type) { type_ = type; }

 private:
  download::DownloadItem::DownloadCreationType type_ =
      download::DownloadItem::DownloadCreationType::TYPE_ACTIVE_DOWNLOAD;
};

IN_PROC_BROWSER_TEST_F(SavePackageFinishedObserverBrowserTest, Success) {
  GURL url =
      embedded_test_server()->GetURL("/save_page/brave-text-content.html");
  ui_test_utils::NavigateToURL(browser(), url);
  auto* download_manager = content::BrowserContext::GetDownloadManager(
      web_contents()->GetBrowserContext());
  {
    base::RunLoop run_loop;
    auto completed_callback = base::BindOnce(
        [](base::OnceClosure callback, download::DownloadItem* item) {
          EXPECT_EQ(item->GetState(), download::DownloadItem::COMPLETE);
          EXPECT_EQ(item->GetDownloadCreationType(),
                    download::DownloadItem::TYPE_SAVE_PAGE_AS);
          if (callback)
            std::move(callback).Run();
        },
        run_loop.QuitClosure());
    base::FilePath saved_main_file_path;
    base::FilePath saved_main_directory_path;
    GetDestinationPaths("index", &saved_main_file_path,
                        &saved_main_directory_path);
    SavePackageFinishedObserver finished_observer(
        download_manager, saved_main_file_path, std::move(completed_callback));
    FakePackageDownloadItem item;
    ASSERT_FALSE(finished_observer.HasInProgressDownload(&item));
    item.SetDownloadCreationType(download::DownloadItem::TYPE_SAVE_PAGE_AS);
    ASSERT_FALSE(finished_observer.HasInProgressDownload(&item));
    item.SetTargetFilePath(saved_main_file_path);
    ASSERT_TRUE(finished_observer.HasInProgressDownload(&item));
    ASSERT_FALSE(finished_observer.HasInProgressDownload(nullptr));
    web_contents()->SavePage(saved_main_file_path, saved_main_directory_path,
                             content::SAVE_PAGE_TYPE_AS_COMPLETE_HTML);

    run_loop.Run();
  }
}
