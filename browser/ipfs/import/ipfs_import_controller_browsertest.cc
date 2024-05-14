/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/import/ipfs_import_controller.h"

#include "base/path_service.h"
#include "brave/browser/ipfs/ipfs_blob_context_getter_factory.h"
#include "brave/browser/ipfs/ipfs_dns_resolver_impl.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/browser/ipfs/ipfs_service_impl_delegate.h"
#include "brave/browser/ipfs/ipfs_tab_helper.h"
#include "brave/components/ipfs/blob_context_getter_factory.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "chrome/browser/notifications/notification_display_service_tester.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/channel_info.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"

using content::WebContents;

namespace {

class FakeIpfsService : public ipfs::IpfsService {
 public:
  FakeIpfsService(
      PrefService* prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      ipfs::BlobContextGetterFactoryPtr blob_context_getter_factory,
      const base::FilePath& user_dir,
      version_info::Channel channel)
      : ipfs::IpfsService(
            prefs,
            url_loader_factory,
            std::move(blob_context_getter_factory),
            user_dir,
            channel,
            std::make_unique<ipfs::IpfsDnsResolverImpl>(),
            std::make_unique<ipfs::IpfsServiceImplDelegate>(prefs)) {}
  ~FakeIpfsService() override = default;
  void ImportTextToIpfs(const std::string& text,
                        const std::string& host,
                        ipfs::ImportCompletedCallback callback) override {
    function_calls_["ImportTextToIpfs"]++;
    if (callback)
      std::move(callback).Run(data_);
  }
  void ImportLinkToIpfs(const GURL& url,
                        ipfs::ImportCompletedCallback callback) override {
    function_calls_["ImportLinkToIpfs"]++;
    if (callback)
      std::move(callback).Run(data_);
  }
  void ImportFileToIpfs(const base::FilePath& path,
                        const std::string& key,
                        ipfs::ImportCompletedCallback callback) override {
    function_calls_["ImportFileToIpfs"]++;
    if (callback)
      std::move(callback).Run(data_);
  }
  void ImportDirectoryToIpfs(const base::FilePath& path,
                             const std::string& key,
                             ipfs::ImportCompletedCallback callback) override {
    function_calls_["ImportDirectoryToIpfs"]++;
    if (callback)
      std::move(callback).Run(data_);
    if (directory_callback_)
      std::move(directory_callback_).Run();
  }
  void PreWarmShareableLink(const GURL& url) override {
    function_calls_["PreWarmShareableLink"]++;
  }
  int GetCallsNumber(const std::string& function) {
    if (!function_calls_.count(function))
      return 0;
    return function_calls_[function];
  }
  void SetImportData(const ipfs::ImportedData& data) { data_ = data; }
  void SetDirectoryCallback(base::OnceClosure callback) {
    directory_callback_ = std::move(callback);
  }

 private:
  ipfs::ImportedData data_;
  base::OnceClosure directory_callback_;
  std::unordered_map<std::string, int> function_calls_;
};

}  // namespace

namespace ipfs {

class IpfsImportControllerBrowserTest : public InProcessBrowserTest {
 public:
  IpfsImportControllerBrowserTest() = default;
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    ASSERT_TRUE(embedded_test_server()->Start());
    display_service_ = std::make_unique<NotificationDisplayServiceTester>(
        Profile::FromBrowserContext(active_contents()->GetBrowserContext()));
    base::FilePath user_dir = base::FilePath(FILE_PATH_LITERAL("test"));
    auto context_getter = std::make_unique<IpfsBlobContextGetterFactory>(
        active_contents()->GetBrowserContext());
    fake_service_ = std::make_unique<FakeIpfsService>(
        nullptr, nullptr, std::move(context_getter), user_dir,
        chrome::GetChannel());
  }

  content::WebContents* active_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  bool NotificationShown() {
    std::vector<message_center::Notification> notifications =
        display_service_->GetDisplayedNotificationsForType(
            NotificationHandler::Type::SEND_TAB_TO_SELF);
    return notifications.size() == 1u;
  }

  FakeIpfsService* fake_ipfs_service() { return fake_service_.get(); }

 private:
  std::unique_ptr<NotificationDisplayServiceTester> display_service_;
  std::unique_ptr<FakeIpfsService> fake_service_;
};

IN_PROC_BROWSER_TEST_F(IpfsImportControllerBrowserTest, ImportFileToIpfs) {
  ASSERT_TRUE(
      ipfs::IPFSTabHelper::MaybeCreateForWebContents(active_contents()));
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_contents());
  if (!helper)
    return;
  base::FilePath user_dir = base::FilePath(FILE_PATH_LITERAL("test"));
  auto* ipfs_service = fake_ipfs_service();
  ipfs::ImportedData data;
  data.hash = "QmYbK4SLaSvTKKAKvNZMwyzYPy4P3GqBPN6CZzbS73FxxU";
  data.filename = "google.com";
  data.size = 111;
  data.directory = "/brave/imports/";
  data.state = ipfs::IPFS_IMPORT_SUCCESS;
  ipfs_service->SetImportData(data);
  auto* controller = helper->GetImportController();
  controller->SetIpfsServiceForTesting(ipfs_service);
  EXPECT_EQ(browser()->tab_strip_model()->GetTabCount(), 1);
  controller->ImportFileToIpfs(base::FilePath(FILE_PATH_LITERAL("fake.file")),
                               std::string());
  EXPECT_EQ(browser()->tab_strip_model()->GetTabCount(), 2);
  auto* web_content = browser()->tab_strip_model()->GetWebContentsAt(1);
  ASSERT_TRUE(web_content);
  GURL url =
      ipfs::ResolveWebUIFilesLocation(data.directory, chrome::GetChannel());
  EXPECT_EQ(web_content->GetURL().spec(), url.spec());
  EXPECT_EQ(ipfs_service->GetCallsNumber("ImportFileToIpfs"), 1);
  EXPECT_EQ(ipfs_service->GetCallsNumber("PreWarmShareableLink"), 1);
  ASSERT_TRUE(NotificationShown());
}

IN_PROC_BROWSER_TEST_F(IpfsImportControllerBrowserTest, ImportTextToIpfs) {
  ASSERT_TRUE(
      ipfs::IPFSTabHelper::MaybeCreateForWebContents(active_contents()));
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_contents());
  if (!helper)
    return;
  base::FilePath user_dir = base::FilePath(FILE_PATH_LITERAL("test"));
  auto* ipfs_service = fake_ipfs_service();
  ipfs::ImportedData data;
  data.hash = "QmYbK4SLaSvTKKAKvNZMwyzYPy4P3GqBPN6CZzbS73FxxU";
  data.filename = "google.com";
  data.size = 111;
  data.directory = "/brave/imports/";
  data.state = ipfs::IPFS_IMPORT_SUCCESS;
  ipfs_service->SetImportData(data);
  auto* controller = helper->GetImportController();
  controller->SetIpfsServiceForTesting(ipfs_service);
  EXPECT_EQ(browser()->tab_strip_model()->GetTabCount(), 1);
  controller->ImportTextToIpfs("test");
  EXPECT_EQ(browser()->tab_strip_model()->GetTabCount(), 2);
  auto* web_content = browser()->tab_strip_model()->GetWebContentsAt(1);
  ASSERT_TRUE(web_content);
  GURL url =
      ipfs::ResolveWebUIFilesLocation(data.directory, chrome::GetChannel());
  EXPECT_EQ(web_content->GetURL().spec(), url.spec());
  EXPECT_EQ(ipfs_service->GetCallsNumber("ImportTextToIpfs"), 1);
  EXPECT_EQ(ipfs_service->GetCallsNumber("PreWarmShareableLink"), 1);
  ASSERT_TRUE(NotificationShown());
}

IN_PROC_BROWSER_TEST_F(IpfsImportControllerBrowserTest, ImportLinkToIpfs) {
  ASSERT_TRUE(
      ipfs::IPFSTabHelper::MaybeCreateForWebContents(active_contents()));
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_contents());
  if (!helper)
    return;
  base::FilePath user_dir = base::FilePath(FILE_PATH_LITERAL("test"));
  auto* ipfs_service = fake_ipfs_service();
  ipfs::ImportedData data;
  data.hash = "QmYbK4SLaSvTKKAKvNZMwyzYPy4P3GqBPN6CZzbS73FxxU";
  data.filename = "google.com";
  data.size = 111;
  data.directory = "/brave/imports/";
  data.state = ipfs::IPFS_IMPORT_SUCCESS;
  ipfs_service->SetImportData(data);
  auto* controller = helper->GetImportController();
  controller->SetIpfsServiceForTesting(ipfs_service);
  EXPECT_EQ(browser()->tab_strip_model()->GetTabCount(), 1);
  controller->ImportLinkToIpfs(GURL("test.com"));
  EXPECT_EQ(browser()->tab_strip_model()->GetTabCount(), 2);
  auto* web_content = browser()->tab_strip_model()->GetWebContentsAt(1);
  ASSERT_TRUE(web_content);
  GURL url =
      ipfs::ResolveWebUIFilesLocation(data.directory, chrome::GetChannel());
  EXPECT_EQ(web_content->GetURL().spec(), url.spec());
  EXPECT_EQ(ipfs_service->GetCallsNumber("ImportLinkToIpfs"), 1);
  EXPECT_EQ(ipfs_service->GetCallsNumber("PreWarmShareableLink"), 1);
  ASSERT_TRUE(NotificationShown());
}

IN_PROC_BROWSER_TEST_F(IpfsImportControllerBrowserTest, ImportDirectoryToIpfs) {
  ASSERT_TRUE(
      ipfs::IPFSTabHelper::MaybeCreateForWebContents(active_contents()));
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_contents());
  if (!helper)
    return;
  base::FilePath user_dir = base::FilePath(FILE_PATH_LITERAL("test"));

  auto* ipfs_service = fake_ipfs_service();
  ipfs::ImportedData data;
  data.hash = "QmYbK4SLaSvTKKAKvNZMwyzYPy4P3GqBPN6CZzbS73FxxU";
  data.filename = "google.com";
  data.size = 111;
  data.directory = "/brave/imports/";
  data.state = ipfs::IPFS_IMPORT_SUCCESS;
  ipfs_service->SetImportData(data);
  auto* controller = helper->GetImportController();
  controller->SetIpfsServiceForTesting(ipfs_service);
  EXPECT_EQ(browser()->tab_strip_model()->GetTabCount(), 1);
  controller->ImportDirectoryToIpfs(
      base::FilePath(FILE_PATH_LITERAL("test.file")), std::string());
  EXPECT_EQ(browser()->tab_strip_model()->GetTabCount(), 2);
  auto* web_content = browser()->tab_strip_model()->GetWebContentsAt(1);
  ASSERT_TRUE(web_content);
  GURL url =
      ipfs::ResolveWebUIFilesLocation(data.directory, chrome::GetChannel());
  EXPECT_EQ(web_content->GetURL().spec(), url.spec());
  EXPECT_EQ(ipfs_service->GetCallsNumber("ImportDirectoryToIpfs"), 1);
  EXPECT_EQ(ipfs_service->GetCallsNumber("PreWarmShareableLink"), 1);
  ASSERT_TRUE(NotificationShown());
}

IN_PROC_BROWSER_TEST_F(IpfsImportControllerBrowserTest,
                       ImportCurrentPageToIpfs) {
  ASSERT_TRUE(
      ipfs::IPFSTabHelper::MaybeCreateForWebContents(active_contents()));
  ipfs::IPFSTabHelper* helper =
      ipfs::IPFSTabHelper::FromWebContents(active_contents());
  if (!helper)
    return;
  base::FilePath user_dir = base::FilePath(FILE_PATH_LITERAL("test"));
  auto* ipfs_service = fake_ipfs_service();
  ipfs::ImportedData data;
  data.hash = "QmYbK4SLaSvTKKAKvNZMwyzYPy4P3GqBPN6CZzbS73FxxU";
  data.filename = "google.com";
  data.size = 111;
  data.directory = "/brave/imports/";
  data.state = ipfs::IPFS_IMPORT_SUCCESS;
  ipfs_service->SetImportData(data);
  base::RunLoop run_loop;
  ipfs_service->SetDirectoryCallback(run_loop.QuitClosure());
  auto* controller = helper->GetImportController();
  controller->SetIpfsServiceForTesting(ipfs_service);
  EXPECT_EQ(browser()->tab_strip_model()->GetTabCount(), 1);
  controller->ImportCurrentPageToIpfs();
  run_loop.Run();
  EXPECT_EQ(browser()->tab_strip_model()->GetTabCount(), 2);
  auto* web_content = browser()->tab_strip_model()->GetWebContentsAt(1);
  ASSERT_TRUE(web_content);
  GURL url =
      ipfs::ResolveWebUIFilesLocation(data.directory, chrome::GetChannel());
  EXPECT_EQ(web_content->GetURL().spec(), url.spec());
  EXPECT_EQ(ipfs_service->GetCallsNumber("ImportDirectoryToIpfs"), 1);
  EXPECT_EQ(ipfs_service->GetCallsNumber("PreWarmShareableLink"), 1);
  ASSERT_TRUE(NotificationShown());
}

}  // namespace ipfs
