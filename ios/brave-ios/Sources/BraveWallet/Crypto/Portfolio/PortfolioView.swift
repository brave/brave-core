// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import DesignSystem
import Introspect
import Preferences
import Shared
import SnapKit
import Strings
import SwiftUI
import UIKit

struct PortfolioView: View {

  var cryptoStore: CryptoStore
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var portfolioStore: PortfolioStore

  @Environment(\.walletActionDestination)
  private var walletActionDestination: Binding<WalletActionDestination?>

  @State private var selectedContent: PortfolioSegmentedControl.Item = .assets
  @ObservedObject private var isShowingNFTsTab = Preferences.Wallet.isShowingNFTsTab

  @State private var isPresentingEditUserAssets: Bool = false
  @State private var isPresentingAssetsFilters: Bool = false
  @State private var isPresentingAddCustomNFT: Bool = false
  @State private var isPresentingNFTsFilters: Bool = false
  @State private var bitcoinBalanceDetails: BitcoinBalanceDetails?

  var body: some View {
    ScrollView {
      VStack(spacing: 0) {
        PortfolioHeaderView(
          keyringStore: keyringStore,
          walletActionDestination: walletActionDestination,
          selectedDateRange: $portfolioStore.timeframe,
          balance: portfolioStore.balance,
          balanceDifference: portfolioStore.balanceDifference,
          historicalBalances: portfolioStore.historicalBalances,
          isLoading: portfolioStore.isLoadingBalances
        )
        contentDrawer
      }
    }
    .onAppear {
      portfolioStore.update()
    }
    .background(
      VStack(spacing: 0) {
        Color(braveSystemName: .pageBackground)  // top scroll rubberband area
        Color(braveSystemName: .containerBackground)  // bottom drawer scroll rubberband area
      }.edgesIgnoringSafeArea(.all)
    )
    .background(
      Color.clear
        .sheet(isPresented: $isPresentingEditUserAssets) {
          EditUserAssetsView(
            networkStore: networkStore,
            keyringStore: keyringStore,
            userAssetsStore: cryptoStore.openUserAssetsStore()
          )
          .onDisappear {
            cryptoStore.closeUserAssetsStore()
          }
        }
    )
    .background(
      Color.clear
        .sheet(isPresented: $isPresentingAssetsFilters) {
          FiltersDisplaySettingsView(
            filters: portfolioStore.filters,
            isNFTFilters: false,
            networkStore: networkStore,
            save: { filters in
              portfolioStore.saveFilters(filters)
            }
          )
          .presentationDetents([
            .fraction(0.7),
            .large,
          ])
        }
    )
    .background(
      Color.clear
        .sheet(isPresented: $isPresentingAddCustomNFT) {
          AddCustomAssetView(
            networkStore: networkStore,
            networkSelectionStore: networkStore.openNetworkSelectionStore(mode: .formSelection),
            keyringStore: keyringStore,
            userAssetStore: cryptoStore.openUserAssetsStore(),
            supportedTokenTypes: [.nft]
          )
          .onDisappear {
            cryptoStore.closeUserAssetsStore()
          }
        }
    )
    .background(
      Color.clear
        .sheet(isPresented: $isPresentingNFTsFilters) {
          FiltersDisplaySettingsView(
            filters: cryptoStore.nftStore.filters,
            isNFTFilters: true,
            networkStore: networkStore,
            save: { filters in
              cryptoStore.nftStore.saveFilters(filters)
            }
          )
          .presentationDetents([
            .fraction(0.6),
            .large,
          ])
        }
    )
    .background(
      Color.clear
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
    )
  }

  private var contentDrawer: some View {
    VStack {
      if isShowingNFTsTab.value {
        PortfolioSegmentedControl(selected: $selectedContent)
          .padding(.horizontal)
          .padding(.bottom, 6)
      }
      Group {
        if selectedContent == .assets || !isShowingNFTsTab.value {
          PortfolioAssetsView(
            cryptoStore: cryptoStore,
            keyringStore: keyringStore,
            networkStore: networkStore,
            portfolioStore: portfolioStore,
            isPresentingEditUserAssets: $isPresentingEditUserAssets,
            isPresentingFilters: $isPresentingAssetsFilters,
            bitcoinBalanceDetails: $bitcoinBalanceDetails
          )
          .padding(.horizontal, 8)
        } else {
          NFTView(
            cryptoStore: cryptoStore,
            keyringStore: keyringStore,
            networkStore: cryptoStore.networkStore,
            nftStore: cryptoStore.nftStore,
            isPresentingFilters: $isPresentingNFTsFilters,
            isPresentingAddCustomNFT: $isPresentingAddCustomNFT
          )
          .padding(.horizontal, 8)
        }
      }
    }
    .padding(.vertical)
    .background(
      ZStack {
        Color(braveSystemName: .pageBackground)  // bg behind rounded corners
          .zIndex(0)
        Color(braveSystemName: .containerBackground)
          .roundedCorner(16, corners: [.topLeft, .topRight])
          .shadow(color: .black.opacity(0.04), radius: 8, x: 0, y: -8)
          .zIndex(1)
      }
    )
  }
}

/// Builds the in-section header for `Assets`/`NFT` that is shown in expanded and non-expanded state. Not used for ungrouped assets.
struct PortfolioAssetGroupHeaderView: View {
  let group: any WalletAssetGroupViewModel

  var body: some View {
    VStack(spacing: 0) {
      HStack {
        if case .network(let networkInfo) = group.groupType {
          NetworkIconView(network: networkInfo, length: 32)
        } else if case .account(let accountInfo) = group.groupType {
          Blockie(address: accountInfo.blockieSeed)
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
      }
      .padding(.vertical, 4)
    }
  }
}

#if DEBUG
struct PortfolioViewController_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      PortfolioView(
        cryptoStore: .previewStore,
        keyringStore: .previewStore,
        networkStore: .previewStore,
        portfolioStore: CryptoStore.previewStore.portfolioStore
      )
      .navigationBarTitleDisplayMode(.inline)
    }
    .previewColorSchemes()
  }
}
#endif

private struct RoundedRect: Shape {
  var radius: CGFloat
  var corners: UIRectCorner

  func path(in rect: CGRect) -> Path {
    let path = UIBezierPath(
      roundedRect: rect,
      byRoundingCorners: corners,
      cornerRadii: CGSize(
        width: radius,
        height: radius
      )
    )
    return Path(path.cgPath)
  }
}

extension View {
  func roundedCorner(_ radius: CGFloat, corners: UIRectCorner) -> some View {
    clipShape(RoundedRect(radius: radius, corners: corners))
  }
}
