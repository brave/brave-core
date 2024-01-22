/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import SwiftUI
import BraveCore
import SnapKit
import Introspect
import Strings
import DesignSystem
import BraveUI
import Shared
import Preferences

struct PortfolioView: View {
  
  var cryptoStore: CryptoStore
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var portfolioStore: PortfolioStore
  
  @Environment(\.buySendSwapDestination)
  private var buySendSwapDestination: Binding<BuySendSwapDestination?>
  
  @State private var selectedContent: PortfolioSegmentedControl.Item = .assets
  @ObservedObject private var isShowingNFTsTab = Preferences.Wallet.isShowingNFTsTab
  
  var body: some View {
    ScrollView {
      VStack(spacing: 0) {
        PortfolioHeaderView(
          keyringStore: keyringStore,
          buySendSwapDestination: buySendSwapDestination,
          selectedDateRange: $portfolioStore.timeframe,
          balance: portfolioStore.balance,
          balanceDifference: portfolioStore.balanceDifference,
          historicalBalances: portfolioStore.historicalBalances,
          isLoading: portfolioStore.isLoadingBalances
        )
        contentDrawer
      }
    }
    .background(
      VStack(spacing: 0) {
        Color(braveSystemName: .pageBackground) // top scroll rubberband area
        Color(braveSystemName: .containerBackground) // bottom drawer scroll rubberband area
      }.edgesIgnoringSafeArea(.all)
    )
  }
  
  private var contentDrawer: some View {
    LazyVStack {
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
            portfolioStore: portfolioStore
          )
          .padding(.horizontal, 8)
        } else {
          NFTView(
            cryptoStore: cryptoStore,
            keyringStore: keyringStore,
            networkStore: cryptoStore.networkStore,
            nftStore: cryptoStore.nftStore
          )
          .padding(.horizontal, 8)
        }
      }
    }
    .padding(.vertical)
    .background(
      ZStack {
        Color(braveSystemName: .pageBackground) // bg behind rounded corners
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
        if case let .network(networkInfo) = group.groupType {
          NetworkIconView(network: networkInfo, length: 32)
        } else if case let .account(accountInfo) = group.groupType {
          Blockie(address: accountInfo.address)
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
    clipShape(RoundedRect(radius: radius, corners: corners) )
  }
}
