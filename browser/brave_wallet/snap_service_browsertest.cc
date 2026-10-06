/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap_service.h"

#include <optional>
#include <string>
#include <tuple>

#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/web_ui_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "url/gurl.h"

namespace brave_wallet {

namespace {
constexpr char kTestSnapId[] = "npm:test-snap";
constexpr char kHangSnapId[] = "npm:hang-snap";
constexpr char kWarmupSnapId[] = "npm:warmup-snap";
constexpr char kTestPassword[] = "brave1234";

// Id of the hidden snaps-container iframe the wallet page injects. Must match
// kSnapHostFrameId in snap_host_frame.ts.
constexpr char kSnapHostFrameId[] = "snaps-container-frame";
}  // namespace

// Shared plumbing; subclasses only pick the feature configuration.
class SnapServiceBrowserTestBase : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    ASSERT_TRUE(service());
    service()->SetSnapBundleForTesting(kTestSnapId, ReadTestSnapBundle());
  }

  std::tuple<bool, std::optional<std::string>, std::optional<std::string>>
  LoadSnap(const std::string& snap_id) {
    base::test::TestFuture<bool, const std::optional<std::string>&,
                           const std::optional<std::string>&>
        future;
    service()->LoadSnap(
        snap_id, future.GetCallback<bool, const std::optional<std::string>&,
                                    const std::optional<std::string>&>());
    return future.Take();
  }

  void CreateWallet() {
    ASSERT_TRUE(keyring_service());
    base::test::TestFuture<const std::optional<std::string>&> future;
    keyring_service()->CreateWallet(
        kTestPassword, future.GetCallback<const std::optional<std::string>&>());
    ASSERT_TRUE(future.Wait());
  }

  void CreateAndLockWallet() {
    CreateWallet();
    keyring_service()->Lock();
  }

  BraveWalletService* wallet_service() {
    return BraveWalletServiceFactory::GetServiceForContext(
        browser()->GetProfile());
  }

  SnapService* service() {
    auto* service = wallet_service();
    return service ? service->snap_service() : nullptr;
  }

  KeyringService* keyring_service() {
    auto* service = wallet_service();
    return service ? service->keyring_service() : nullptr;
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
};

// Runs each test against both execution environments. GetParam() is true for
// the hidden-WebContents host.
class SnapServiceBrowserTest : public SnapServiceBrowserTestBase,
                               public testing::WithParamInterface<bool> {
 public:
  SnapServiceBrowserTest() {
    feature_list_.InitAndEnableFeatureWithParameters(
        features::kBraveWalletSnapFeature,
        {{"use_hidden_webcontents", GetParam() ? "true" : "false"}});
  }

  // Wallet-page env needs a visible tab. The hidden env starts its host
  // lazily on the first LoadSnap, so warm it up instead of polling first.
  void EnsureBridgeBound() {
    if (!GetParam()) {
      ASSERT_TRUE(
          ui_test_utils::NavigateToURL(browser(), GURL(kBraveUIWalletURL)));
    } else {
      service()->SetSnapBundleForTesting(kWarmupSnapId, "module.exports=''");
      auto [success, error, result] = LoadSnap(kWarmupSnapId);
      ASSERT_TRUE(success);
    }
    ASSERT_TRUE(base::test::RunUntil(
        [&] { return service()->IsBridgeBoundForTesting(); }));
  }
};

INSTANTIATE_TEST_SUITE_P(All, SnapServiceBrowserTest, testing::Bool());

// Snaps hosted by the wallet page's hidden iframe (the default).
class SnapServiceWalletPageBrowserTest : public SnapServiceBrowserTestBase {
 public:
  SnapServiceWalletPageBrowserTest() {
    feature_list_.InitAndEnableFeature(features::kBraveWalletSnapFeature);
  }

  void OpenWalletPage() {
    ASSERT_TRUE(
        ui_test_utils::NavigateToURL(browser(), GURL(kBraveUIWalletURL)));
    ASSERT_TRUE(base::test::RunUntil(
        [&] { return service()->IsBridgeBoundForTesting(); }));
  }
};

// Snaps hosted by a browser-owned hidden WebContents.
class SnapServiceHiddenWebContentsBrowserTest
    : public SnapServiceBrowserTestBase {
 public:
  SnapServiceHiddenWebContentsBrowserTest() {
    feature_list_.InitAndEnableFeatureWithParameters(
        features::kBraveWalletSnapFeature,
        {{"use_hidden_webcontents", "true"}});
  }
};

class SnapServiceFeatureDisabledBrowserTest : public InProcessBrowserTest {
 public:
  SnapServiceFeatureDisabledBrowserTest() {
    feature_list_.InitAndDisableFeature(features::kBraveWalletSnapFeature);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(SnapServiceFeatureDisabledBrowserTest,
                       ServiceNotCreatedWhenFeatureDisabled) {
  auto* wallet_service =
      BraveWalletServiceFactory::GetServiceForContext(browser()->GetProfile());
  ASSERT_TRUE(wallet_service);
  EXPECT_EQ(nullptr, wallet_service->snap_service());
}

IN_PROC_BROWSER_TEST_P(SnapServiceBrowserTest,
                       LoadSnapSucceedsWhenBundlePresent) {
  EnsureBridgeBound();

  auto [success, error, result] = LoadSnap(kTestSnapId);
  EXPECT_TRUE(success);
  EXPECT_FALSE(error.has_value());
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(kTestSnapId, *result);
}

IN_PROC_BROWSER_TEST_P(SnapServiceBrowserTest, UnknownSnapReturnsError) {
  EnsureBridgeBound();

  auto [success, error, result] = LoadSnap("npm:missing");
  EXPECT_FALSE(success);
  ASSERT_TRUE(error.has_value());
  EXPECT_EQ("Bundle not found", *error);
  EXPECT_FALSE(result.has_value());
}

IN_PROC_BROWSER_TEST_P(SnapServiceBrowserTest, LockedWalletFailsFast) {
  EnsureBridgeBound();
  CreateAndLockWallet();

  auto [success, error, result] = LoadSnap(kTestSnapId);
  EXPECT_FALSE(success);
  ASSERT_TRUE(error.has_value());
  EXPECT_EQ("Wallet is locked", *error);
  EXPECT_FALSE(result.has_value());
}

IN_PROC_BROWSER_TEST_F(SnapServiceWalletPageBrowserTest,
                       LoadSnapReplyOnNavigationAway) {
  OpenWalletPage();

  // Infinite loop so the mojo call stays pending until the bridge disconnects.
  service()->SetSnapBundleForTesting(kHangSnapId, "while (true) {}");

  base::test::TestFuture<bool, const std::optional<std::string>&,
                         const std::optional<std::string>&>
      future;
  service()->LoadSnap(
      kHangSnapId, future.GetCallback<bool, const std::optional<std::string>&,
                                      const std::optional<std::string>&>());

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("chrome://newtab")));

  auto [success, error, result] = future.Take();
  EXPECT_FALSE(success);
  ASSERT_TRUE(error.has_value());
  EXPECT_FALSE(result.has_value());
}

IN_PROC_BROWSER_TEST_F(SnapServiceWalletPageBrowserTest,
                       LoadSnapFailsWhenWalletPageIsNotRunning) {
  auto [success, error, result] = LoadSnap(kTestSnapId);
  EXPECT_FALSE(success);
  ASSERT_TRUE(error.has_value());
  EXPECT_EQ("Wallet page is not running", *error);
  EXPECT_FALSE(result.has_value());
}

IN_PROC_BROWSER_TEST_F(SnapServiceWalletPageBrowserTest,
                       BridgeDisconnectOnPageClose) {
  OpenWalletPage();

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("chrome://newtab")));
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return !service()->IsBridgeBoundForTesting(); }));

  auto [success, error, result] = LoadSnap(kTestSnapId);
  EXPECT_FALSE(success);
  ASSERT_TRUE(error.has_value());
  EXPECT_EQ("Wallet page is not running", *error);
  EXPECT_FALSE(result.has_value());
}

// Locking drops the browser's remote. The still-open page must notice the
// disconnect and re-bind, otherwise snaps stay broken until a reload. Not
// asserting the intermediate unbound state on purpose: the page re-binds as
// soon as it sees the error, so that window isn't reliably observable.
IN_PROC_BROWSER_TEST_F(SnapServiceWalletPageBrowserTest,
                       BridgeRebindsAfterLock) {
  OpenWalletPage();
  CreateAndLockWallet();

  base::test::TestFuture<bool> unlock_future;
  keyring_service()->Unlock(kTestPassword, unlock_future.GetCallback());
  ASSERT_TRUE(unlock_future.Take());

  ASSERT_TRUE(base::test::RunUntil(
      [&] { return service()->IsBridgeBoundForTesting(); }));

  auto [success, error, result] = LoadSnap(kTestSnapId);
  EXPECT_TRUE(success);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(kTestSnapId, *result);
}

IN_PROC_BROWSER_TEST_F(SnapServiceHiddenWebContentsBrowserTest,
                       LoadSnapSucceedsWithNoWalletTab) {
  auto [success, error, result] = LoadSnap(kTestSnapId);
  EXPECT_TRUE(success);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(kTestSnapId, *result);
}

// LoadSnap fails fast on a locked wallet, so no host is ever created.
IN_PROC_BROWSER_TEST_F(SnapServiceHiddenWebContentsBrowserTest,
                       LockedWalletDoesNotStartHost) {
  CreateAndLockWallet();

  auto [success, error, result] = LoadSnap(kTestSnapId);
  ASSERT_FALSE(success);
  ASSERT_TRUE(error.has_value());
  EXPECT_EQ("Wallet is locked", *error);
  EXPECT_FALSE(service()->IsBridgeBoundForTesting());
}

IN_PROC_BROWSER_TEST_F(SnapServiceHiddenWebContentsBrowserTest,
                       LockTearsDownHost) {
  auto [success, error, result] = LoadSnap(kTestSnapId);
  ASSERT_TRUE(success);
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return service()->IsBridgeBoundForTesting(); }));

  CreateAndLockWallet();

  ASSERT_TRUE(base::test::RunUntil(
      [&] { return !service()->IsBridgeBoundForTesting(); }));
}

// With the hidden host running, the wallet page must not inject its own
// snaps-container frame — a second host would steal the profile-wide bridge
// and orphan the snaps already loaded into the hidden host.
IN_PROC_BROWSER_TEST_F(SnapServiceHiddenWebContentsBrowserTest,
                       WalletPageDoesNotStealBridge) {
  auto [success1, error1, result1] = LoadSnap(kTestSnapId);
  ASSERT_TRUE(success1);

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL(kBraveUIWalletURL)));
  auto* wallet_page = browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(false,
            content::EvalJs(wallet_page,
                            content::JsReplace("!!document.getElementById($1)",
                                               kSnapHostFrameId)));

  auto [success2, error2, result2] = LoadSnap(kTestSnapId);
  EXPECT_TRUE(success2);
  ASSERT_TRUE(result2.has_value());
  EXPECT_EQ(kTestSnapId, *result2);
}

}  // namespace brave_wallet
