/* Copyright 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI
import DesignSystem
import BraveCore

struct PortfolioAssetsView: View {

  var cryptoStore: CryptoStore
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var portfolioStore: PortfolioStore

  @State private var isPresentingEditUserAssets: Bool = false
  @State private var isPresentingFiltersDisplaySettings: Bool = false
  @State private var selectedToken: BraveWallet.BlockchainToken?
  @State private var groupToggleState: [AssetGroupViewModel.ID: Bool] = [:]

  var body: some View {
    LazyVStack(spacing: 16) {
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
      editUserAssetsButton
        .padding(.trailing, 10)
      filtersButton
    }
    .frame(maxWidth: .infinity, alignment: .leading)
    .padding(.horizontal)
  }

  private var editUserAssetsButton: some View {
    AssetButton(braveSystemName: "leo.list.settings", action: {
      isPresentingEditUserAssets = true
    })
    .sheet(isPresented: $isPresentingEditUserAssets) {
      EditUserAssetsView(
        networkStore: networkStore,
        keyringStore: keyringStore,
        userAssetsStore: portfolioStore.userAssetsStore
      ) {
        cryptoStore.updateAssets()
      }
    }
  }

  private var filtersButton: some View {
    AssetButton(braveSystemName: "leo.filter.settings", action: {
      isPresentingFiltersDisplaySettings = true
    })
    .sheet(isPresented: $isPresentingFiltersDisplaySettings) {
      FiltersDisplaySettingsView(
        filters: portfolioStore.filters,
        isNFTFilters: false,
        networkStore: networkStore,
        save: { filters in
          portfolioStore.saveFilters(filters)
        }
      )
      .osAvailabilityModifiers({ view in
        if #available(iOS 16, *) {
          view
            .presentationDetents([
              .fraction(0.7),
              .large
            ])
        } else {
          view
        }
      })
    }
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
          quantity: asset.quantity
        )
      }
    }
    .padding(.horizontal)
  }
  
  /// Builds the expandable/collapseable (expanded by default) section content for a given group.
  @ViewBuilder private func groupedAssetsSection(for group: AssetGroupViewModel) -> some View {
    WalletDisclosureGroup(
      isExpanded: Binding(
        get: { groupToggleState[group.id, default: true] },
        set: { isExpanded in
          groupToggleState[group.id] = isExpanded
        }
      ),
      content: {
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
              quantity: asset.quantity
            )
          }
        }
      },
      label: {
        if case let .account(account) = group.groupType {
          AddressView(address: account.address) {
            groupHeader(for: group)
          }
        } else {
          groupHeader(for: group)
        }
      }
    )
  }
  
  /// Builds the in-section header for an AssetGroupViewModel that is shown in expanded and non-expanded state. Not used for ungrouped assets.
  private func groupHeader(for group: AssetGroupViewModel) -> some View {
    VStack(spacing: 0) {
      HStack {
        if case let .network(networkInfo) = group.groupType {
          NetworkIcon(network: networkInfo, length: 32)
        } else if case let .account(accountInfo) = group.groupType {
          Blockie(address: accountInfo.address, shape: .rectangle)
            .frame(width: 32, height: 32)
            .clipShape(RoundedRectangle(cornerRadius: 4))
        }
        VStack(alignment: .leading) {
          Text(group.title)
            .font(.callout.weight(.semibold))
            .foregroundColor(Color(WalletV2Design.textPrimary))
          if let description = group.description {
            Text(description)
              .font(.footnote)
              .foregroundColor(Color(WalletV2Design.textSecondary))
          }
        }
        .multilineTextAlignment(.leading)
        Spacer()
        Text(portfolioStore.currencyFormatter.string(from: NSNumber(value: group.totalFiatValue)) ?? "")
          .font(.callout.weight(.semibold))
          .foregroundColor(Color(WalletV2Design.textPrimary))
          .multilineTextAlignment(.trailing)
      }
      .padding(.vertical, 4)
    }
  }
}
