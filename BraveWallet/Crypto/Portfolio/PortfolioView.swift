/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import SwiftUI
import BraveCore
import SnapKit
import Introspect
import struct Shared.Strings

struct Currency {
  var image: UIImage
  var name: String
  var symbol: String
  var cost: Double
}

struct PortfolioView: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var portfolioStore: PortfolioStore
  
  @State private var dismissedBackupBannerThisSession: Bool = false
  @State private var isPresentingBackup: Bool = false
  @State private var isPresentingEditUserAssets: Bool = false
  
  private var isShowingBackupBanner: Bool {
    !keyringStore.keyring.isBackedUp && !dismissedBackupBannerThisSession
  }
  
  private let currencyFormatter = NumberFormatter().then {
    $0.numberStyle = .currency
    $0.currencyCode = "USD"
  }
  
  private var listHeader: some View {
    VStack(spacing: 0) {
      if isShowingBackupBanner {
        BackupNotifyView(action: {
          isPresentingBackup = true
        }, onDismiss: {
          // Animating this doesn't seem to work in SwiftUI.. will keep an eye out for iOS 15
          dismissedBackupBannerThisSession = true
        })
        .buttonStyle(PlainButtonStyle())
        .padding([.top, .leading, .trailing], 12)
        .sheet(isPresented: $isPresentingBackup) {
          NavigationView {
            BackupRecoveryPhraseView(keyringStore: keyringStore)
          }
          .environment(\.modalPresentationMode, $isPresentingBackup)
        }
      }
      BalanceHeaderView(
        balance: portfolioStore.balance,
        historicalBalances: portfolioStore.historicalBalances,
        isLoading: portfolioStore.isLoadingBalances,
        networkStore: networkStore,
        selectedDateRange: $portfolioStore.timeframe
      )
    }
  }
  
  @State private var tableInset: CGFloat = -16.0
  
  var body: some View {
    List {
      Section(
        header: listHeader
          .padding(.horizontal, tableInset) // inset grouped layout margins workaround
          .resetListHeaderStyle()
      ) {
      }
      Section(
        header: WalletListHeaderView(title: Text(Strings.Wallet.assetsTitle))
      ) {
        ForEach(portfolioStore.userVisibleAssets) { asset in
          PortfolioAssetView(
            image: AssetIconView(token: asset.token),
            title: asset.token.name,
            symbol: asset.token.symbol,
            amount: currencyFormatter.string(from: NSNumber(value: (Double(asset.price) ?? 0) * asset.decimalBalance)) ?? "",
            quantity: String(format: "%.04f", asset.decimalBalance)
          )
        }
        Button(action: { isPresentingEditUserAssets = true }) {
          Text(Strings.Wallet.editVisibleAssetsButtonTitle)
            .multilineTextAlignment(.center)
            .font(.footnote.weight(.semibold))
            .foregroundColor(Color(.bravePrimary))
            .frame(maxWidth: .infinity)
        }
        .sheet(isPresented: $isPresentingEditUserAssets) {
          EditUserAssetsView(userAssetsStore: portfolioStore.userAssetsStore) {
            portfolioStore.update()
          }
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .animation(.default, value: portfolioStore.userVisibleAssets)
    .listStyle(InsetGroupedListStyle())
    .introspectTableView { tableView in
      tableInset = -tableView.layoutMargins.left
    }
  }
}

struct BalanceHeaderView: View {
  var balance: String
  var historicalBalances: [BalanceTimePrice]
  var isLoading: Bool
  @ObservedObject var networkStore: NetworkStore
  @Binding var selectedDateRange: BraveWallet.AssetPriceTimeframe
  
  @Environment(\.sizeCategory) private var sizeCategory
  @Environment(\.horizontalSizeClass) private var horizontalSizeClass
  
  @State private var selectedBalance: BalanceTimePrice?
  
  private var balanceOrDataPointView: some View {
    HStack {
      Group {
        if sizeCategory.isAccessibilityCategory {
          VStack(alignment: .leading) {
            NetworkPicker(
              networks: networkStore.ethereumChains,
              selectedNetwork: networkStore.selectedChainBinding
            )
            Text(verbatim: balance)
          }
        } else {
          HStack {
            Text(verbatim: balance)
            NetworkPicker(
              networks: networkStore.ethereumChains,
              selectedNetwork: networkStore.selectedChainBinding
            )
            Spacer()
          }
        }
      }
      .opacity(selectedBalance == nil ? 1 : 0)
      .overlay(
        Group {
          if let dataPoint = selectedBalance {
            Text(dataPoint.formattedPrice)
          }
        },
        alignment: .leading
      )
      if horizontalSizeClass == .regular {
        Spacer()
        DateRangeView(selectedRange: $selectedDateRange)
          .padding(6)
          .overlay(
            RoundedRectangle(cornerRadius: 10, style: .continuous)
              .strokeBorder(Color(.secondaryButtonTint))
          )
      }
    }
    .font(.largeTitle.bold())
    .foregroundColor(.primary)
    .padding(.top, 12)
  }
  
  var body: some View {
    VStack(alignment: .leading, spacing: 4) {
      balanceOrDataPointView
      LineChartView(data: historicalBalances, numberOfColumns: historicalBalances.count, selectedDataPoint: $selectedBalance) {
        LinearGradient(braveGradient: .lightGradient02)
          .shimmer(isLoading)
      }
      .frame(height: 148)
      .padding(.horizontal, -12)
      .animation(.default, value: historicalBalances)
      if horizontalSizeClass == .compact {
        DateRangeView(selectedRange: $selectedDateRange)
      }
    }
    .padding(12)
  }
}

#if DEBUG
struct PortfolioViewController_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      PortfolioView(
        keyringStore: WalletStore.previewStore.keyringStore,
        networkStore: WalletStore.previewStore.networkStore,
        portfolioStore: WalletStore.previewStore.portfolioStore
      )
      .navigationBarTitleDisplayMode(.inline)
    }
      .previewColorSchemes()
  }
}
#endif
