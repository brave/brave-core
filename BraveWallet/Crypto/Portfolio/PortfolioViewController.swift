/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import SwiftUI
import BraveCore
import SnapKit

struct Currency {
  var image: UIImage
  var name: String
  var symbol: String
  var cost: Double
}

struct Candle: DataPoint, Equatable {
  var value: CGFloat
}

@available(iOS 14.0, *)
struct PortfolioView: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: EthNetworkStore
  
  @State private var dismissedBackupBannerThisSession: Bool = false
  @State private var isPresentingBackup: Bool = false
  
  private var isShowingBackupBanner: Bool {
    !keyringStore.keyring.isBackedUp && !dismissedBackupBannerThisSession
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
        balance: "$12,453.17",
        networkStore: networkStore
      )
    }
  }
  
  var body: some View {
    List {
      Section(
        header: listHeader
          .padding(.horizontal, -16) // inset grouped layout margins workaround
          .listRowInsets(.zero)
          .textCase(.none)
      ) {
      }
      Section(
        header: WalletListHeaderView(title: Text("Assets"))
      ) {
        PortfolioAssetView(image: .init(), title: "Ethereum", symbol: "ETH", amount: "$10,810.03", quantity: "8")
        PortfolioAssetView(image: .init(), title: "Basic Attention Token", symbol: "BAT", amount: "$4,510.03", quantity: "500")
        Button(action: { }) {
          Text("Edit Visible Assets")
            .multilineTextAlignment(.center)
            .font(.footnote.weight(.semibold))
            .foregroundColor(Color(.bravePrimary))
            .frame(maxWidth: .infinity)
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .listStyle(InsetGroupedListStyle())
  }
}

@available(iOS 14.0, *)
struct BalanceHeaderView: View {
  var balance: String
  @ObservedObject var networkStore: EthNetworkStore
  
  @Environment(\.sizeCategory) private var sizeCategory
  @Environment(\.horizontalSizeClass) private var horizontalSizeClass
  @State private var selectedDateRange: BraveWallet.AssetPriceTimeframe = .oneDay
  @State private var selectedCandle: Candle?
  
  var data: [Candle] {
    switch selectedDateRange {
    case .oneDay:
      return [10, 20, 30, 20, 10, 40, 50, 80, 100].map(Candle.init)
    case .live:
      return [10, 20, 30, 20, 10, 40, 50, 80, 100].map(Candle.init).reversed()
    case .oneWeek:
      return [10, 20, 30, 20, 10].map(Candle.init)
    case .oneMonth:
      return [10, 20, 30, 20, 10, 40, 50, 80, 100, 200, 100, 120].map(Candle.init)
    case .threeMonths:
      return [10, 20, 30, 20, 10, 40, 50, 80, 100].map(Candle.init)
    case .oneYear:
      return [10, 20, 30, 20, 10, 40, 50, 80, 100].map(Candle.init)
    case .all:
      return [10, 20, 30, 20, 10, 40, 50, 80, 100].map(Candle.init)
    @unknown default:
      return [10, 20, 30, 20, 10, 40, 50, 80, 100].map(Candle.init)
    }
  }
  
  private var balanceOrDataPointView: some View {
    HStack {
      if let dataPoint = selectedCandle {
        Text(verbatim: "\(dataPoint.value)")
      } else {
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
      HStack(spacing: 4) {
        Image(systemName: "triangle.fill")
          .font(.system(size: 8))
          .foregroundColor(.green)
        Text(verbatim: "1.3%")
          .foregroundColor(.green)
        Text(verbatim: "Today") // NSLocalizedString
          .foregroundColor(.secondary)
      }
      .font(.subheadline)
      .frame(maxWidth: .infinity, alignment: .leading)
      LineChartView(data: data, numberOfColumns: 12, selectedDataPoint: $selectedCandle) {
        LinearGradient(braveGradient: .lightGradient02)
      }
      .frame(height: 148)
      .padding(.horizontal, -12)
      .animation(.default, value: data)
      if horizontalSizeClass == .compact {
        DateRangeView(selectedRange: $selectedDateRange)
      }
    }
    .padding(12)
  }
}

#if DEBUG
@available(iOS 14.0, *)
struct PortfolioViewController_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      PortfolioView(keyringStore: .previewStore, networkStore: .previewStore)
        .navigationBarTitleDisplayMode(.inline)
    }
      .previewColorSchemes()
  }
}
#endif
