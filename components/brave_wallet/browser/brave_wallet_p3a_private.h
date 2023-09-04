/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_P3A_PRIVATE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_P3A_PRIVATE_H_

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

class BraveWalletP3APrivate : public mojom::BraveWalletP3A {
 public:
  BraveWalletP3APrivate() = default;

  void ReportJSProvider(mojom::JSProviderType provider_type,
                        mojom::CoinType coin_type,
                        bool allow_provider_overwrite) override;
  void ReportOnboardingAction(
      mojom::OnboardingAction onboarding_action) override;
  void ReportTransactionSent(mojom::CoinType coin, bool new_send) override;
  void RecordActiveWalletCount(int count, mojom::CoinType coin_type) override;
  void RecordNFTGalleryView(int nft_count) override;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_P3A_PRIVATE_H_
