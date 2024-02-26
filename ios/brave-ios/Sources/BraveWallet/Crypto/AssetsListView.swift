// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import SwiftUI

struct AssetsListView: View {

  let assets: [AssetViewModel]
  let currencyFormatter: NumberFormatter
  let selectedAsset: (BraveWallet.BlockchainToken) -> Void

  var body: some View {
    ScrollView {
      LazyVStack {
        if assets.isEmpty {
          emptyAssetsState
        } else {
          ForEach(assets) { asset in
            Button {
              selectedAsset(asset.token)
            } label: {
              PortfolioAssetView(
                image: AssetIconView(
                  token: asset.token,
                  network: asset.network,
                  shouldShowNetworkIcon: true
                ),
                title: asset.token.name,
                symbol: asset.token.symbol,
                networkName: asset.network.chainName,
                amount: asset.fiatAmount(currencyFormatter: currencyFormatter),
                quantity: asset.quantity,
                shouldHideBalance: true
              )
            }
          }
        }
      }
      .padding()
    }
    .background(Color(braveSystemName: .containerBackground))
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
