/* Copyright 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI
import DesignSystem
import BraveCore
import Preferences

struct PortfolioAssetsView: View {

  var cryptoStore: CryptoStore
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var portfolioStore: PortfolioStore

  @Binding var isPresentingEditUserAssets: Bool
  @Binding var isPresentingFilters: Bool
  @State private var selectedToken: BraveWallet.BlockchainToken?
  @State private var groupToggleState: [AssetGroupViewModel.ID: Bool] = [:]
  @ObservedObject private var isShowingBalances = Preferences.Wallet.isShowingBalances

  var body: some View {
    VStack(spacing: 16) {
      assetSectionsHeader
      
      if portfolioStore.isShowingAssetsLoadingState {
        SkeletonLoadingAssetView()
      } else if portfolioStore.isShowingAssetsEmptyState {
        emptyAssetsState
      } else {
        ForEach(portfolioStore.assetGroups) { group in
          if group.groupType == .none {
            ungroupedAssets(group)
          } else {
            groupedAssetsSection(for: group)
          }
        }
      }
    }
    .background(
      NavigationLink(
        isActive: Binding(
          get: { selectedToken != nil },
          set: { if !$0 { selectedToken = nil } }
        ),
        destination: {
          if let token = selectedToken {
            AssetDetailView(
              assetDetailStore: cryptoStore.assetDetailStore(for: .blockchainToken(token)),
              keyringStore: keyringStore,
              networkStore: cryptoStore.networkStore
            )
            .onDisappear {
              cryptoStore.closeAssetDetailStore(for: .blockchainToken(token))
            }
          }
        },
        label: {
          EmptyView()
        })
    )
  }

  /// Header for the assets section(s) containing the title, edit user assets and filter buttons
  private var assetSectionsHeader: some View {
    HStack {
      Text(Strings.Wallet.assetsTitle)
        .foregroundColor(Color(braveSystemName: .textPrimary))
        .font(.title3.weight(.semibold))
      if portfolioStore.isLoadingDiscoverAssets {
        ProgressView()
          .padding(.leading, 5)
      }
      Spacer()
      WalletIconButton(braveSystemName: "leo.list.settings", action: {
        isPresentingEditUserAssets = true
      })
      .padding(.trailing, 10)
      WalletIconButton(braveSystemName: "leo.filter.settings", action: {
        isPresentingFilters = true
      })
    }
    .frame(maxWidth: .infinity, alignment: .leading)
    .padding(.horizontal)
  }

  private var emptyAssetsState: some View {
    VStack(spacing: 10) {
      Image("portfolio-empty", bundle: .module)
        .aspectRatio(contentMode: .fit)
      Text(Strings.Wallet.portfolioEmptyStateTitle)
        .font(.headline)
        .foregroundColor(Color(WalletV2Design.textPrimary))
      Text(Strings.Wallet.portfolioEmptyStateDescription)
        .font(.footnote)
        .foregroundColor(Color(WalletV2Design.textSecondary))
    }
    .multilineTextAlignment(.center)
    .padding(.vertical)
  }
  
  /// Builds the list of assets without any grouping or expandable / collapse behaviour.
  @ViewBuilder private func ungroupedAssets(_ group: AssetGroupViewModel) -> some View {
    ForEach(group.assets) { asset in
      Button(action: {
        selectedToken = asset.token
      }) {
        PortfolioAssetView(
          image: AssetIconView(
            token: asset.token,
            network: asset.network,
            shouldShowNetworkIcon: true
          ),
          title: asset.token.name,
          symbol: asset.token.symbol,
          networkName: asset.network.chainName,
          amount: asset.fiatAmount(currencyFormatter: portfolioStore.currencyFormatter),
          quantity: asset.quantity,
          shouldHideBalance: true
        )
      }
    }
    .padding(.horizontal)
  }
  
  /// Builds the expandable/collapseable (expanded by default) section content for a given group.
  @ViewBuilder private func groupedAssetsSection(for group: AssetGroupViewModel) -> some View {
    WalletDisclosureGroup(
      isNFTGroup: false,
      isExpanded: Binding(
        get: { groupToggleState[group.id, default: true] },
        set: { isExpanded in
          groupToggleState[group.id] = isExpanded
        }
      ),
      content: {
        LazyVStack(spacing: 8) {
          ForEach(group.assets) { asset in
            Button(action: {
              selectedToken = asset.token
            }) {
              PortfolioAssetView(
                image: AssetIconView(
                  token: asset.token,
                  network: asset.network,
                  shouldShowNetworkIcon: true
                ),
                title: asset.token.name,
                symbol: asset.token.symbol,
                networkName: asset.network.chainName,
                amount: asset.fiatAmount(currencyFormatter: portfolioStore.currencyFormatter),
                quantity: asset.quantity,
                shouldHideBalance: true
              )
            }
          }
        }
      },
      label: {
        if case let .account(account) = group.groupType {
          AddressView(address: account.address) {
            PortfolioAssetGroupHeaderView(group: group)
          }
        } else {
          PortfolioAssetGroupHeaderView(group: group)
        }
      }
    )
  }
}
