/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "net/dns/mock_host_resolver.h"

namespace brave_wallet {

namespace {

void OnGetActiveOrigin(bool* callback_called,
                       const std::string& expected_active_origin,
                       const std::string& active_origin) {
  EXPECT_EQ(expected_active_origin, active_origin);
  *callback_called = true;
}

}  // namespace

class TestBraveWalletServiceObserver
    : public brave_wallet::mojom::BraveWalletServiceObserver {
 public:
  TestBraveWalletServiceObserver() {}

  void OnDefaultWalletChanged(mojom::DefaultWallet default_wallet) override {}

  void OnActiveOriginChanged(const std::string& origin) override {
    active_origin_ = origin;
  }

  std::string active_origin() { return active_origin_; }

  mojo::PendingRemote<brave_wallet::mojom::BraveWalletServiceObserver>
  GetReceiver() {
    return observer_receiver_.BindNewPipeAndPassRemote();
  }

  void Reset() { active_origin_ = ""; }

 private:
  std::string active_origin_;
  mojo::Receiver<brave_wallet::mojom::BraveWalletServiceObserver>
      observer_receiver_{this};
};

class BraveWalletServiceTest : public InProcessBrowserTest {
 public:
  BraveWalletServiceTest() {
    feature_list_.InitAndEnableFeature(
        brave_wallet::features::kNativeBraveWalletFeature);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");

    https_server_.reset(new net::EmbeddedTestServer(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS));
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    ASSERT_TRUE(https_server_->Start());

    wallet_service_ = brave_wallet::BraveWalletServiceFactory::GetInstance()
                          ->GetServiceForContext(browser()->profile());
  }

  BraveWalletService* wallet_service() { return wallet_service_; }
  net::EmbeddedTestServer* https_server() { return https_server_.get(); }

 private:
  BraveWalletService* wallet_service_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveWalletServiceTest, ActiveOrigin) {
  GURL url = https_server()->GetURL("a.com", "/simple.html");
  std::string expected_origin = url.DeprecatedGetOriginAsURL().spec();
  TestBraveWalletServiceObserver observer;
  wallet_service()->AddObserver(observer.GetReceiver());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  bool callback_called = false;
  wallet_service()->GetActiveOrigin(
      base::BindOnce(&OnGetActiveOrigin, &callback_called, expected_origin));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(observer.active_origin(), expected_origin);

  url = https_server()->GetURL("b.com", "/simple.html");
  expected_origin = url.DeprecatedGetOriginAsURL().spec();
  callback_called = false;
  observer.Reset();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  wallet_service()->GetActiveOrigin(
      base::BindOnce(&OnGetActiveOrigin, &callback_called, expected_origin));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(observer.active_origin(), expected_origin);

  url = https_server()->GetURL("c.com", "/simple.html");
  expected_origin = url.DeprecatedGetOriginAsURL().spec();
  observer.Reset();
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  wallet_service()->GetActiveOrigin(
      base::BindOnce(&OnGetActiveOrigin, &callback_called, expected_origin));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(observer.active_origin(), expected_origin);

  url = https_server()->GetURL("d.com", "/simple.html");
  expected_origin = url.DeprecatedGetOriginAsURL().spec();
  observer.Reset();
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_WINDOW,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);
  wallet_service()->GetActiveOrigin(
      base::BindOnce(&OnGetActiveOrigin, &callback_called, expected_origin));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(observer.active_origin(), expected_origin);
}

}  // namespace brave_wallet
