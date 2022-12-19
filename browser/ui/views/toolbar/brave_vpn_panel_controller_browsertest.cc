/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/app/brave_command_ids.h"
#include "brave/components/brave_vpn/common/features.h"
#include "brave/components/skus/common/features.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "content/public/test/browser_test.h"

class BraveVPNPanelControllerTest : public DialogBrowserTest {
 public:
  BraveVPNPanelControllerTest() {
    scoped_feature_list_.InitWithFeatures(
        {skus::features::kSkusFeature, brave_vpn::features::kBraveVPN}, {});
  }

  ~BraveVPNPanelControllerTest() override = default;

  // TestBrowserUi:
  void ShowUi(const std::string& name) override {
    browser()->command_controller()->ExecuteCommand(IDC_SHOW_BRAVE_VPN_PANEL);
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveVPNPanelControllerTest, InvokeUi_Dialog) {
  ShowAndVerifyUi();
}
