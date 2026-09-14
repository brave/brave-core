/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snaps_service.h"

#include <optional>
#include <string>
#include <tuple>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/web_ui_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "url/gurl.h"

namespace brave_wallet {

namespace {
constexpr char kTestSnapId[] = "npm:test-snap";
}  // namespace

class SnapsServiceBrowserTest : public InProcessBrowserTest {
 public:
  SnapsServiceBrowserTest() {
    feature_list_.InitAndEnableFeature(features::kBraveWalletSnapsFeature);
  }

  void SetUpOnMainThread() override {
    auto* wallet_service = BraveWalletServiceFactory::GetServiceForContext(
        browser()->GetProfile());
    ASSERT_TRUE(wallet_service);
    service_ = wallet_service->snaps_service();
    ASSERT_TRUE(service_);
    service_->SetSnapBundleForTesting(kTestSnapId, ReadTestSnapBundle());
  }

  void OpenWalletPage() {
    ASSERT_TRUE(
        ui_test_utils::NavigateToURL(browser(), GURL(kBraveUIWalletURL)));
    ASSERT_TRUE(base::test::RunUntil(
        [&] { return service()->IsBridgeBoundForTesting(); }));
  }

  std::tuple<bool, std::optional<std::string>, std::optional<std::string>>
  LoadSnap(const std::string& snap_id) {
    base::test::TestFuture<bool, const std::optional<std::string>&,
                           const std::optional<std::string>&>
        future;
    service_->LoadSnap(snap_id, future.GetCallback());
    auto [success, error, result] = future.Take();
    return {success, error, result};
  }

  SnapsService* service() { return service_; }

 private:
  base::test::ScopedFeatureList feature_list_;
  raw_ptr<SnapsService> service_ = nullptr;
};

IN_PROC_BROWSER_TEST_F(SnapsServiceBrowserTest,
                       LoadSnapFailsWhenWalletPageIsNotRunning) {
  auto [success, error, result] = LoadSnap(kTestSnapId);
  EXPECT_FALSE(success);
  ASSERT_TRUE(error.has_value());
  EXPECT_EQ("Wallet page is not running", *error);
  EXPECT_FALSE(result.has_value());
}

IN_PROC_BROWSER_TEST_F(SnapsServiceBrowserTest,
                       LoadSnapSucceedsWhenBundlePresent) {
  OpenWalletPage();

  auto [success, error, result] = LoadSnap(kTestSnapId);
  EXPECT_TRUE(success);
  EXPECT_FALSE(error.has_value());
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(kTestSnapId, *result);
}

IN_PROC_BROWSER_TEST_F(SnapsServiceBrowserTest, UnknownSnapReturnsError) {
  OpenWalletPage();

  auto [success, error, result] = LoadSnap("npm:missing");
  EXPECT_FALSE(success);
  ASSERT_TRUE(error.has_value());
  EXPECT_EQ("Bundle not found", *error);
  EXPECT_FALSE(result.has_value());
}

IN_PROC_BROWSER_TEST_F(SnapsServiceBrowserTest, BridgeDisconnectOnPageClose) {
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

}  // namespace brave_wallet
