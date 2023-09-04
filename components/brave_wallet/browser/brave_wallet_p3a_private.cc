/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_p3a_private.h"

namespace brave_wallet {

void BraveWalletP3APrivate::ReportJSProvider(
    mojom::JSProviderType provider_type,
    mojom::CoinType coin_type,
    bool allow_provider_overwrite) {}

void BraveWalletP3APrivate::ReportOnboardingAction(
    mojom::OnboardingAction onboarding_action) {}

void BraveWalletP3APrivate::ReportTransactionSent(mojom::CoinType coin,
                                                  bool new_send) {}

void BraveWalletP3APrivate::RecordActiveWalletCount(int count,
                                                    mojom::CoinType coin_type) {
}

void BraveWalletP3APrivate::RecordNFTGalleryView(int nft_count) {}

}  // namespace brave_wallet
