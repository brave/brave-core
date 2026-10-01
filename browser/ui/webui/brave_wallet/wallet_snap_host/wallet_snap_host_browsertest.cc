/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>
#include <string>
#include <tuple>

#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_snap_host/wallet_snap_host_ui.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/snap_service.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/web_ui_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/webui_config_map.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "url/gurl.h"

namespace brave_wallet {

namespace {
constexpr char kTestSnapId[] = "npm:test-snap";
constexpr char kSnapHostFrameId[] = "wallet-snap-host-frame";
}  // namespace

class WalletSnapHostBrowserTest : public InProcessBrowserTest {
 public:
  WalletSnapHostBrowserTest() {
    feature_list_.InitAndEnableFeature(features::kBraveWalletSnapFeature);
  }

  SnapService* service() {
    auto* wallet_service = BraveWalletServiceFactory::GetServiceForContext(
        browser()->GetProfile());
    return wallet_service ? wallet_service->snap_service() : nullptr;
  }

  std::tuple<bool, std::optional<std::string>, std::optional<std::string>>
  LoadSnap(const std::string& snap_id) {
    base::test::TestFuture<bool, const std::optional<std::string>&,
                           const std::optional<std::string>&>
        future;
    service()->LoadSnap(snap_id, future.GetCallback());
    return future.Take();
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

class WalletSnapHostFeatureDisabledBrowserTest : public InProcessBrowserTest {
 public:
  WalletSnapHostFeatureDisabledBrowserTest() {
    feature_list_.InitAndDisableFeature(features::kBraveWalletSnapFeature);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(WalletSnapHostBrowserTest,
                       DirectNavigationBindsSnapService) {
  ASSERT_TRUE(service());
  service()->SetSnapBundleForTesting(kTestSnapId, ReadTestSnapBundle());

  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL(kBraveUIWalletSnapHostURL)));
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return service()->IsBridgeBoundForTesting(); }));

  auto [success, error, result] = LoadSnap(kTestSnapId);
  EXPECT_TRUE(success);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(kTestSnapId, *result);
}

// Proves frame-src + frame-ancestors + XFO all pass and Mojo works in an
// iframed trusted WebUI.
IN_PROC_BROWSER_TEST_F(WalletSnapHostBrowserTest,
                       NestedFrameInWalletPageBindsSnapService) {
  ASSERT_TRUE(service());
  service()->SetSnapBundleForTesting(kTestSnapId, ReadTestSnapBundle());

  auto* wallet_rfh =
      ui_test_utils::NavigateToURL(browser(), GURL(kBraveUIWalletURL));
  ASSERT_TRUE(wallet_rfh);
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return service()->IsBridgeBoundForTesting(); }));

  auto* host_rfh = content::ChildFrameAt(wallet_rfh, 0);
  ASSERT_TRUE(host_rfh);
  EXPECT_EQ(GURL(kBraveUIWalletSnapHostURL), host_rfh->GetLastCommittedURL());
  EXPECT_FALSE(host_rfh->IsErrorDocument());

  auto [success, error, result] = LoadSnap(kTestSnapId);
  EXPECT_TRUE(success);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(kTestSnapId, *result);
}

IN_PROC_BROWSER_TEST_F(WalletSnapHostBrowserTest, HostFrameIsSingleton) {
  auto* wallet_rfh =
      ui_test_utils::NavigateToURL(browser(), GURL(kBraveUIWalletURL));
  ASSERT_TRUE(wallet_rfh);
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return service() && service()->IsBridgeBoundForTesting(); }));

  EXPECT_EQ(
      1, content::EvalJs(wallet_rfh, content::JsReplace(
                                         "document.querySelectorAll($1).length",
                                         std::string("#") + kSnapHostFrameId))
             .ExtractInt());
}

IN_PROC_BROWSER_TEST_F(WalletSnapHostFeatureDisabledBrowserTest,
                       ConfigNotRegisteredWhenFeatureDisabled) {
  WalletSnapHostUIConfig config;
  EXPECT_FALSE(config.IsWebUIEnabled(browser()->GetProfile()));
  EXPECT_EQ(nullptr,
            content::WebUIConfigMap::GetInstance().GetConfig(
                browser()->GetProfile(), GURL(kBraveUIWalletSnapHostURL)));
}

IN_PROC_BROWSER_TEST_F(WalletSnapHostFeatureDisabledBrowserTest,
                       NoHostFrameWhenFeatureDisabled) {
  auto* rfh = ui_test_utils::NavigateToURL(browser(), GURL(kBraveUIWalletURL));
  ASSERT_TRUE(rfh);
  EXPECT_EQ(nullptr, content::ChildFrameAt(rfh, 0));
}

}  // namespace brave_wallet
