// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore

#if DEBUG

class TestBraveWalletP3A: BraveWalletBraveWalletP3A {
  var _reportProvider:
    (
      (
        _ providerType: BraveWallet.JSProviderType, _ coinType: BraveWallet.CoinType,
        _ allowProviderOverwrite: Bool
      ) -> Void
    )?
  func reportJsProvider(
    providerType: BraveWallet.JSProviderType,
    coinType: BraveWallet.CoinType,
    allowProviderOverwrite: Bool
  ) {
    _reportProvider?(providerType, coinType, allowProviderOverwrite)
  }

  var _reportOnboarding: ((_ onboardingAction: BraveWallet.OnboardingAction) -> Void)?
  func reportOnboardingAction(_ onboardingAction: BraveWallet.OnboardingAction) {
    _reportOnboarding?(onboardingAction)
  }

  var _reportTransactionSent: ((_ coin: BraveWallet.CoinType, _ newSend: Bool) -> Void)?
  func reportTransactionSent(coin: BraveWallet.CoinType, newSend: Bool) {
    _reportTransactionSent?(coin, newSend)
  }

  var _recordActiveWalletCount: ((_ count: Int32, _ coinType: BraveWallet.CoinType) -> Void)?
  func recordActiveWalletCount(_ count: Int32, coinType: BraveWallet.CoinType) {
    _recordActiveWalletCount?(count, coinType)
  }

  var _recordNftGalleryView: ((_ nftCount: Int32) -> Void)?
  func recordNftGalleryView(nftCount: Int32) {
    _recordNftGalleryView?(nftCount)
  }
}

#endif
