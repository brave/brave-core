// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import SwiftUI

struct AssetsListView: View {

  let assets: [AssetViewModel]
  /// If a container should be shown to around unavailable balance banner
  /// for Bitcoin asset row (when there is a pending balance)
  let shouldShowContainerForBitcoin: Bool
  let currencyFormatter: NumberFormatter
  @State private var bitcoinBalanceDetails: BitcoinBalanceDetails?
  let selectedAsset: (BraveWallet.BlockchainToken) -> Void

  var body: some View {
    ScrollView {
      LazyVStack {
        if assets.isEmpty {
          emptyAssetsState
        } else {
          ForEach(assets) { asset in
            FungibleAssetButton(
              asset: asset,
              shouldShowContainerForBitcoin: shouldShowContainerForBitcoin,
              currencyFormatter: currencyFormatter,
              bitcoinBalanceDetails: $bitcoinBalanceDetails,
              action: selectedAsset
            )
          }
        }
      }
      .padding(.vertical)
    }
    .background(Color(braveSystemName: .containerBackground))
    .sheet(
      isPresented: Binding(
        get: { bitcoinBalanceDetails != nil },
        set: {
          if !$0 {
            bitcoinBalanceDetails = nil
          }
        }
      )
    ) {
      if let bitcoinBalanceDetails {
        BTCBalanceDetailsView(
          details: bitcoinBalanceDetails,
          currencyFormatter: .usdCurrencyFormatter
        )
      }
    }
  }

  private var emptyAssetsState: some View {
    VStack(spacing: 10) {
      Image("portfolio-empty", bundle: .module)
        .aspectRatio(contentMode: .fit)
      Text(Strings.Wallet.portfolioEmptyStateTitle)
        .font(.headline)
        .foregroundColor(Color(WalletV2Design.textPrimary))
    }
    .multilineTextAlignment(.center)
    .padding(.vertical)
  }
}
