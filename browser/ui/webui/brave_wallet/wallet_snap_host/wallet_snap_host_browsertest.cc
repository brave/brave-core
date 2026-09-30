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
#include "content/public/browser/webui_config_map.h"
#include "content/public/test/browser_test.h"
#include "url/gurl.h"

namespace brave_wallet {

namespace {
constexpr char kTestSnapId[] = "npm:test-snap";
}  // namespace

class WalletSnapHostBrowserTest : public InProcessBrowserTest {
 public:
  WalletSnapHostBrowserTest() {
    feature_list_.InitAndEnableFeatureWithParameters(
        features::kBraveWalletSnapFeature,
        {{"execution_environment", "hidden-web-contents"}});
  }

  SnapService* service() {
    auto* wallet_service = BraveWalletServiceFactory::GetServiceForContext(
        browser()->GetProfile());
    return wallet_service ? wallet_service->snap_service() : nullptr;
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
                       NavigatingBindsSnapServiceAndLoadsBundle) {
  ASSERT_TRUE(service());
  service()->SetSnapBundleForTesting(kTestSnapId, ReadTestSnapBundle());

  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL(kBraveUIWalletSnapHostURL)));
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return service()->IsBridgeBoundForTesting(); }));

  base::test::TestFuture<bool, const std::optional<std::string>&,
                         const std::optional<std::string>&>
      future;
  service()->LoadSnap(
      kTestSnapId, future.GetCallback<bool, const std::optional<std::string>&,
                                      const std::optional<std::string>&>());
  auto [success, error, result] = future.Take();
  EXPECT_TRUE(success);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(kTestSnapId, *result);
}

IN_PROC_BROWSER_TEST_F(WalletSnapHostFeatureDisabledBrowserTest,
                       ConfigNotRegisteredWhenFeatureDisabled) {
  WalletSnapHostUIConfig config;
  EXPECT_FALSE(config.IsWebUIEnabled(browser()->GetProfile()));
  EXPECT_EQ(nullptr,
            content::WebUIConfigMap::GetInstance().GetConfig(
                browser()->GetProfile(), GURL(kBraveUIWalletSnapHostURL)));
}

}  // namespace brave_wallet
