/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_P3A_PRIVATE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_P3A_PRIVATE_H_

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

class BraveWalletP3APrivate : public mojom::BraveWalletP3A {
 public:
  BraveWalletP3APrivate() = default;

  void ReportEthereumProvider(
      mojom::EthereumProviderType provider_type) override;
  void ReportOnboardingAction(
      mojom::OnboardingAction onboarding_action) override;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_P3A_PRIVATE_H_
