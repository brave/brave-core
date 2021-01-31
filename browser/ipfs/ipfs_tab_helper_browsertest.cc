/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/scoped_observer.h"
#include "brave/browser/ipfs/ipfs_tab_helper.h"
#include "brave/common/brave_paths.h"
#include "brave/components/ipfs/features.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/pref_names.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "components/infobars/core/infobar.h"
#include "components/infobars/core/infobar_manager.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

namespace {

std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
    const net::test_server::HttpRequest& request) {
  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse());
  http_response->set_code(net::HTTP_OK);
  http_response->set_content_type("text/html");
  http_response->set_content("test");
  http_response->AddCustomHeader(
      "x-ipfs-path", "/ipfs/QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR");
  return std::move(http_response);
}

}  // namespace

class IPFSTabHelperTest : public InProcessBrowserTest,
                          public infobars::InfoBarManager::Observer {
 public:
  IPFSTabHelperTest() : infobar_observer_(this), infobar_added_(false) {
    feature_list_.InitAndEnableFeature(ipfs::features::kIpfsFeature);
  }

  void WaitForInfobarAdded() {
    if (infobar_added_) {
      return;
    }
    infobar_added_run_loop_ = std::make_unique<base::RunLoop>();
    infobar_added_run_loop_->Run();
  }

  void WaitForTabCount(int expected) {
    while (browser()->tab_strip_model()->count() != expected)
      base::RunLoop().RunUntilIdle();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->RegisterRequestHandler(
        base::BindRepeating(&HandleRequest));
    ASSERT_TRUE(embedded_test_server()->Start());
    ipfs::SetIPFSDefaultGatewayForTest(
        embedded_test_server()->GetURL("cloudflare-ipfs.com", "/"));
  }

  ~IPFSTabHelperTest() override {}

  void AddInfoBarObserver(InfoBarService* infobar_service) {
    infobar_observer_.Add(infobar_service);
  }

  void RemoveInfoBarObserver(InfoBarService* infobar_service) {
    infobar_observer_.Remove(infobar_service);
  }

  content::WebContents* active_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void InfoBarAccept(int expected_buttons) {
    InfoBarService* infobar_service =
        InfoBarService::FromWebContents(active_contents());
    for (size_t i = 0; i < infobar_service->infobar_count(); i++) {
      infobars::InfoBarDelegate* delegate =
          infobar_service->infobar_at(i)->delegate();
      if (delegate->GetIdentifier() ==
          infobars::InfoBarDelegate::IPFS_INFOBAR_DELEGATE) {
        ConfirmInfoBarDelegate* confirm_delegate =
            delegate->AsConfirmInfoBarDelegate();
        ASSERT_EQ(confirm_delegate->GetButtons(), expected_buttons);
        confirm_delegate->Accept();
      }
    }
  }

  void InfoBarCancel(int expected_buttons) {
    InfoBarService* infobar_service =
        InfoBarService::FromWebContents(active_contents());
    for (size_t i = 0; i < infobar_service->infobar_count(); i++) {
      infobars::InfoBarDelegate* delegate =
          infobar_service->infobar_at(i)->delegate();
      if (delegate->GetIdentifier() ==
          infobars::InfoBarDelegate::IPFS_INFOBAR_DELEGATE) {
        ConfirmInfoBarDelegate* confirm_delegate =
            delegate->AsConfirmInfoBarDelegate();
        ASSERT_EQ(confirm_delegate->GetButtons(), expected_buttons);
        confirm_delegate->Cancel();
      }
    }
  }

  bool NavigateToURLUntilLoadStop(const std::string& origin,
                                  const std::string& path) {
    ui_test_utils::NavigateToURL(browser(),
                                 embedded_test_server()->GetURL(origin, path));
    return WaitForLoadStop(active_contents());
  }

 private:
  ScopedObserver<infobars::InfoBarManager, infobars::InfoBarManager::Observer>
      infobar_observer_;
  bool infobar_added_;
  std::unique_ptr<base::RunLoop> infobar_added_run_loop_;
  base::test::ScopedFeatureList feature_list_;

  // infobars::InfoBarManager::Observer:
  void OnInfoBarAdded(infobars::InfoBar* infobar) override {
    if (infobar->delegate()->GetIdentifier() ==
        infobars::InfoBarDelegate::IPFS_INFOBAR_DELEGATE) {
      infobar_added_ = true;
      if (infobar_added_run_loop_) {
        infobar_added_run_loop_->Quit();
      }
    }
  }
};

IN_PROC_BROWSER_TEST_F(IPFSTabHelperTest, InfobarAddWithAccept) {
  InfoBarService* infobar_service =
      InfoBarService::FromWebContents(active_contents());
  AddInfoBarObserver(infobar_service);
  EXPECT_TRUE(NavigateToURLUntilLoadStop(
      "cloudflare-ipfs.com",
      "/ipfs/QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR"));

  WaitForInfobarAdded();
  InfoBarAccept(ConfirmInfoBarDelegate::BUTTON_OK |
                ConfirmInfoBarDelegate::BUTTON_CANCEL);
  // Pref for Wallet should still be ask by default
  auto method = static_cast<ipfs::IPFSResolveMethodTypes>(
      browser()->profile()->GetPrefs()->GetInteger(kIPFSResolveMethod));
  ASSERT_EQ(method, ipfs::IPFSResolveMethodTypes::IPFS_LOCAL);
  RemoveInfoBarObserver(infobar_service);
}

IN_PROC_BROWSER_TEST_F(IPFSTabHelperTest, InfobarAddWithSettings) {
  InfoBarService* infobar_service =
      InfoBarService::FromWebContents(active_contents());
  AddInfoBarObserver(infobar_service);
  EXPECT_TRUE(NavigateToURLUntilLoadStop(
      "cloudflare-ipfs.com",
      "/ipfs/QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR"));

  WaitForInfobarAdded();
  InfoBarCancel(ConfirmInfoBarDelegate::BUTTON_OK |
                ConfirmInfoBarDelegate::BUTTON_CANCEL);
  // Pref for Wallet should still be ask by default
  auto method = static_cast<ipfs::IPFSResolveMethodTypes>(
      browser()->profile()->GetPrefs()->GetInteger(kIPFSResolveMethod));
  ASSERT_EQ(method, ipfs::IPFSResolveMethodTypes::IPFS_ASK);
  WaitForTabCount(2);
  RemoveInfoBarObserver(infobar_service);
}
