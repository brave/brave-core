/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#include "brave/components/brave_vpn/common/features.h"
#include "brave/components/skus/common/features.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "content/public/test/browser_test.h"

class BraveVPNControllerTest : public DialogBrowserTest {
 public:
  BraveVPNControllerTest() {
    scoped_feature_list_.InitWithFeatures(
        {skus::features::kSkusFeature, brave_vpn::features::kBraveVPN}, {});
  }

  ~BraveVPNControllerTest() override = default;

  // TestBrowserUi:
  void ShowUi(const std::string& name) override {
    chrome::BrowserCommandController::From(browser())->ExecuteCommand(
        IDC_SHOW_BRAVE_VPN_PANEL);
  }

 protected:
  static size_t ObserverCount(const brave_vpn::BraveVpnService& service) {
    return service.observers_.size();
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveVPNControllerTest, InvokeUi_Dialog) {
  ShowAndVerifyUi();
}

// The panel's page binds `ServiceHandler` itself, and observes the service
// through it as it loads.
IN_PROC_BROWSER_TEST_F(BraveVPNControllerTest, PanelObservesVpnService) {
  brave_vpn::BraveVpnService* service =
      brave_vpn::BraveVpnServiceFactory::GetForProfile(GetProfile());
  ASSERT_TRUE(service);
  // Native observers, such as the toolbar button's, are already registered.
  const size_t native_observers = ObserverCount(*service);

  ShowUi("");

  EXPECT_TRUE(base::test::RunUntil(
      [&] { return ObserverCount(*service) > native_observers; }));
}
