/* Copyright 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI
import DesignSystem
import BraveCore
import Preferences

struct PortfolioHeaderView: View {
  
  @ObservedObject var keyringStore: KeyringStore
  @Binding var buySendSwapDestination: BuySendSwapDestination?
  @Binding var selectedDateRange: BraveWallet.AssetPriceTimeframe
  var balance: String
  var balanceDifference: BalanceDifference?
  var historicalBalances: [BalanceTimePrice]
  var isLoading: Bool
  
  @State private var isPresentingBackup = false
  @State private var dismissedBackupBannerThisSession = false
  @State private var selectedBalance: BalanceTimePrice?
  @ObservedObject private var isShowingGraph = Preferences.Wallet.isShowingGraph
  @ObservedObject private var isShowingBalances = Preferences.Wallet.isShowingBalances
  
  @Environment(\.colorScheme) private var colourScheme

  private var isShowingBackupBanner: Bool {
    !keyringStore.isWalletBackedUp && !dismissedBackupBannerThisSession
  }
  
  private var emptyBalanceData: [BalanceTimePrice] {
    // About 300 points added so it doesn't animate funny
    (0..<300).map { _ in .init(date: Date(), price: 0.0, formattedPrice: "") }
  }
  
  var body: some View {
    VStack(spacing: 0) {
      if isShowingBackupBanner {
        backupBanner
        
        Spacer().frame(height: 10)
      }
      
      balanceAndPriceChanges
      
      Spacer().frame(height: 24)
      
      buySendSwapButtons
      
      Spacer().frame(height: 24)
      
      if isShowingGraph.value {
        lineChart
      }
    }
    .padding()
    .frame(maxWidth: .infinity)
    .background(Color(braveSystemName: .pageBackground))
  }
  
  private var backupBanner: some View {
    BackupNotifyView(
      action: {
        isPresentingBackup = true
      },
      onDismiss: {
        // Animating this doesn't seem to work in SwiftUI.. will keep an eye out for iOS 15
        dismissedBackupBannerThisSession = true
      }
    )
    .buttonStyle(PlainButtonStyle())
    .padding([.top, .leading, .trailing], 12)
    .sheet(isPresented: $isPresentingBackup) {
      NavigationView {
        BackupWalletView(
          password: nil,
          keyringStore: keyringStore
        )
      }
      .environment(\.modalPresentationMode, $isPresentingBackup)
      .accentColor(Color(.braveBlurpleTint))
    }
  }
  
  private var balanceAndPriceChanges: some View {
    VStack(spacing: 12) {
      Text(isShowingBalances.value ? balance : "****")
        .frame(maxWidth: .infinity)
        .opacity(selectedBalance == nil ? 1 : 0)
        .overlay(
          Group {
            if let dataPoint = selectedBalance {
              Text(isShowingBalances.value ?  dataPoint.formattedPrice : "****")
            }
          }
        )
        .font(.largeTitle.weight(.medium))
        .multilineTextAlignment(.center)
      
      if let balanceDifference {
        HStack {
          Text(isShowingBalances.value ? balanceDifference.priceDifference : "****")
            .font(.footnote)
            .foregroundColor(Color(braveSystemName: balanceDifference.isBalanceUp ? .systemfeedbackSuccessText : .systemfeedbackErrorText))
          Text(isShowingBalances.value ? balanceDifference.percentageChange : "****")
            .font(.footnote)
            .padding(4)
            .foregroundColor(Color(braveSystemName: balanceDifference.isBalanceUp ? .green50 : .red50))
            .background(Color(braveSystemName: balanceDifference.isBalanceUp ? .green20 : .red20).cornerRadius(4))
        }
      }
    }
  }
  
  private var buySendSwapButtons: some View {
    HStack(spacing: 24) {
      PortfolioHeaderButton(style: .buy) {
        buySendSwapDestination = BuySendSwapDestination(kind: .buy)
      }
      PortfolioHeaderButton(style: .send) {
        buySendSwapDestination = BuySendSwapDestination(kind: .send)
      }
      PortfolioHeaderButton(style: .swap) {
        buySendSwapDestination = BuySendSwapDestination(kind: .swap)
      }
    }
    .padding(.horizontal, 30)
  }
  
  @ViewBuilder private var lineChart: some View {
    VStack(spacing: 0) {
      TimeframeSelector(selectedDateRange: $selectedDateRange)
      let chartData = historicalBalances.isEmpty ? emptyBalanceData : historicalBalances
      LineChartView(data: chartData, numberOfColumns: chartData.count, selectedDataPoint: $selectedBalance) {
        LinearGradient(
          gradient: Gradient(colors: [Color(.braveBlurpleTint).opacity(colourScheme == .dark ? 0.5 : 0.2), .clear]),
          startPoint: .top,
          endPoint: .bottom
        )
        .shimmer(isLoading)
      }
      .chartAccessibility(
        title: Strings.Wallet.portfolioPageTitle,
        dataPoints: chartData
      )
      .disabled(historicalBalances.isEmpty)
      .frame(height: 148)
      .padding(.horizontal, -12)
      .animation(.default, value: historicalBalances)
    }
  }
}

struct PortfolioHeaderButton: View {
  
  enum Style: String, Equatable {
    case buy, send, swap, more
    
    var label: String {
      switch self {
      case .buy: return Strings.Wallet.buy
      case .send: return Strings.Wallet.send
      case .swap: return Strings.Wallet.swap
      case .more: return Strings.Wallet.more
      }
    }
    
    var iconName: String {
      switch self {
      case .buy: return "leo.coins.alt1"
      case .send: return "leo.send"
      case .swap: return "leo.currency.exchange"
      case .more: return "leo.more.horizontal"
      }
    }
  }
  
  let style: Style
  let action: () -> Void
  
  var body: some View {
    Button(action: action) {
      VStack {
        Circle()
          .fill(Color(braveSystemName: .buttonBackground))
          .frame(width: 48, height: 48)
          .overlay(
            Image(braveSystemName: style.iconName)
              .foregroundColor(.white)
              .font(.title3.weight(.semibold))
              .dynamicTypeSize(...DynamicTypeSize.xLarge)
          )
        Text(style.label)
          .font(.footnote.weight(.semibold))
          .foregroundColor(Color(braveSystemName: .textPrimary))
      }
    }
  }
}

struct TimeframeSelector: View {
  @Binding var selectedDateRange: BraveWallet.AssetPriceTimeframe
  
  var body: some View {
    Menu(content: {
      ForEach(BraveWallet.AssetPriceTimeframe.allCases, id: \.self) { range in
        Button(action: { selectedDateRange = range }) {
          HStack {
            Image(braveSystemName: "leo.check.normal")
              .resizable()
              .aspectRatio(contentMode: .fit)
              .hidden(isHidden: selectedDateRange != range)
            Text(verbatim: range.accessibilityLabel)
          }
          .tag(range)
        }
      }
    }, label: {
      HStack(spacing: 4) {
        Text(verbatim: selectedDateRange.accessibilityLabel)
          .font(.footnote.weight(.semibold))
        Image(braveSystemName: "leo.carat.down")
      }
      .foregroundColor(Color(braveSystemName: .textInteractive))
      .padding(.vertical, 6)
      .padding(.horizontal, 12)
      .padding(.trailing, -4) // whitespace on `leo.carat.down` symbol
      .background(
        Capsule()
          .strokeBorder(Color(braveSystemName: .dividerInteractive), lineWidth: 1)
      )
    })
    .transaction { transaction in
      transaction.animation = nil
      transaction.disablesAnimations = true
    }
  }
}
