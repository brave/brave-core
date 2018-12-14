/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/run_loop.h"
#include "base/task/post_task.h"
#include "brave/browser/extensions/brave_extension_functional_test.h"
#include "brave/common/extensions/extension_constants.h"
#include "chrome/browser/plugins/plugin_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/pdf_util.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/resource_context.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"
#include "extensions/common/manifest_handlers/mime_types_handler.h"
#include "net/url_request/url_request_context.h"
#include "testing/gtest/include/gtest/gtest.h"

using content::BrowserThread;
using extensions::ExtensionRegistry;

namespace {

class DownloadManagerObserverHelper
    : public content::DownloadManager::Observer {
 public:
  DownloadManagerObserverHelper(Browser* browser)
      : browser_(browser), download_manager_(nullptr) {
    // Set preference to not ask where to save downloads.
    browser_->profile()->GetPrefs()->SetBoolean(prefs::kPromptForDownload,
                                                false);
  }

  ~DownloadManagerObserverHelper() override {
    if (download_manager_)
      download_manager_->RemoveObserver(this);
  }

  void Observe() {
    // Register a download observer.
    content::WebContents* contents =
        browser_->tab_strip_model()->GetActiveWebContents();
    content::BrowserContext* browser_context = contents->GetBrowserContext();
    download_manager_ =
        content::BrowserContext::GetDownloadManager(browser_context);
    download_manager_->AddObserver(this);
  }

  void CheckForDownload(const GURL& url) {
    // Wait for the url to load and check that it loaded.
    ASSERT_EQ(url, GetLastUrl());
    // Check that one download has been initiated.
    DCHECK(download_manager_);
    std::vector<download::DownloadItem*> downloads;
    download_manager_->GetAllDownloads(&downloads);
    ASSERT_EQ(1u, downloads.size());
    // Cancel download to shut down cleanly.
    downloads[0]->Cancel(false);
  }

  const GURL& GetLastUrl() {
    // Wait until the download has been created.
    download_run_loop_.Run();
    return last_url_;
  }

  // content::DownloadManager::Observer implementation.
  void OnDownloadCreated(content::DownloadManager* manager,
                         download::DownloadItem* item) override {
    last_url_ = item->GetURL();
    download_run_loop_.Quit();
  }

 private:
  Browser* browser_;
  content::DownloadManager* download_manager_;
  content::NotificationRegistrar registrar_;
  base::RunLoop download_run_loop_;
  GURL last_url_;
};

class IOHelper {
public: 
  IOHelper(Browser* browser)
     : finished_(false),
       id_("junk"),
       resource_context_(browser->profile()->GetResourceContext()) {}
  ~IOHelper() = default;

  void GetExtensionIdForPdf() {
    base::PostTaskWithTraitsAndReply(
        FROM_HERE, {BrowserThread::IO},
        base::Bind(&IOHelper::GetExtensionIdForPdfOnIOThread,
                   base::Unretained(this)),
        base::Bind(&IOHelper::OnFinished, base::Unretained(this)));

    if (!finished_)
      run_loop_.Run();
  }

  void GetExtensionIdForPdfOnIOThread() {
    id_ =
        PluginUtils::GetExtensionIdForMimeType(resource_context_, kPDFMimeType);
  }

  void OnFinished() {
    finished_ = true;
    run_loop_.Quit();
  }

  std::string& id() { return id_; }

private:
  std::atomic_bool finished_;
  std::string id_;
  content::ResourceContext* resource_context_;
  base::RunLoop run_loop_;
};

} //namespace

class BravePDFDownloadTest : public extensions::ExtensionFunctionalTest {
public:
  BravePDFDownloadTest() = default;
  ~BravePDFDownloadTest() override = default;

protected:
  bool ShouldEnableInstallVerification() override {
    // Simulates behavior in chrome/browser/extensions/install_verifier.cc's
    // GetExperimentStatus.
#if (defined(OS_WIN) || defined(OS_MACOSX))
    return true;
#else
    return false;
#endif
  }

  void SetDownloadPDFs() {
    DCHECK(browser());
    browser()->profile()->GetPrefs()->SetBoolean(
        prefs::kPluginsAlwaysOpenPdfExternally, true);
  }

  void CheckPDFJSExtensionNotLoaded(
      ExtensionRegistry* registry) const {
    // Verify that the PDFJS extension is not loaded.
    EXPECT_FALSE(
        registry->enabled_extensions().Contains(pdfjs_extension_id));
  }

  void CheckCantHandlePDFs(const ExtensionRegistry* registry) const {
    // Verify that there are no extensions that can handle PDFs, except for the
    // Chromium PDF extension which won't be considered due to what is verified
    // in the VerifyChromiumPDFExntension test below.
    const extensions::ExtensionSet& enabled_extensions =
        registry->enabled_extensions();
    for (const auto& extension : enabled_extensions) {
      MimeTypesHandler* handler = MimeTypesHandler::GetHandler(extension.get());
      if (handler && handler->CanHandleMIMEType(kPDFMimeType))
        ASSERT_STREQ(handler->extension_id().c_str(),
                     extension_misc::kPdfExtensionId);
    }
  }
};

IN_PROC_BROWSER_TEST_F(BravePDFDownloadTest, VerifyChromiumPDFExntension) {
  // On Win and Mac extension install verification puts blacklisted
  // Chromium PDF extension into disabled extensions.
#if (defined(OS_WIN) || defined(OS_MACOSX))
  ExtensionRegistry* registry =
      ExtensionRegistry::Get(browser()->profile());
  EXPECT_TRUE(
    registry->disabled_extensions().Contains(extension_misc::kPdfExtensionId));
#endif
  // On all platforms we modified whitelist to make sure Chromium PDF
  // extension is not considered for PDF handling.
  IOHelper helper(browser());
  helper.GetExtensionIdForPdf();
  EXPECT_EQ(helper.id(), std::string());
}

IN_PROC_BROWSER_TEST_F(BravePDFDownloadTest, DownloadPDF) {
  ExtensionRegistry* registry =
      ExtensionRegistry::Get(browser()->profile());
  CheckPDFJSExtensionNotLoaded(registry);

  // Set preference to always download PDFs and check that we can't handle PDFs.
  SetDownloadPDFs();
  // Since in browser tests the extension is not actually loaded we don't have
  // to wait here to ensure the extension has been unloaded.
  CheckCantHandlePDFs(registry);

  // Register a download observer.
  DownloadManagerObserverHelper helper(browser());
  helper.Observe();
  // Navigate to a pdf.
  GURL url(embedded_test_server()->GetURL("/test.pdf"));
  ui_test_utils::NavigateToURL(browser(), url);
  // Check that one download has been initiated.
  helper.CheckForDownload(url);
}

