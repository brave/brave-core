/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/wallet_common_ui.h"
#include "brave/components/constants/webui_url_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace brave_wallet {

TEST(BraveWalletCommonUIUnitTest, IsBraveWalletOrigin) {
  ASSERT_TRUE(
      IsBraveWalletOrigin(url::Origin::Create(GURL(kBraveUIWalletPanelURL))));
  ASSERT_TRUE(
      IsBraveWalletOrigin(url::Origin::Create(GURL(kBraveUIWalletPageURL))));
  ASSERT_FALSE(IsBraveWalletOrigin(url::Origin::Create(GURL("https://a.com"))));
  ASSERT_FALSE(IsBraveWalletOrigin(url::Origin::Create(GURL())));
}

}  // namespace brave_wallet
