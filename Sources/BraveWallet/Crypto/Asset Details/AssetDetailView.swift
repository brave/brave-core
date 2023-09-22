/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import UIKit
import SwiftUI
import BraveCore
import DesignSystem
import Strings
import Preferences
import BraveUI
import Introspect
import Shared

struct AssetDetailView: View {
  @ObservedObject var assetDetailStore: AssetDetailStore
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore

  @State private var tableInset: CGFloat = -16.0
  @State private var isShowingAddAccount: Bool = false
  @State private var transactionDetails: TransactionDetailsStore?
  @State private var isShowingAuroraBridgeAlert: Bool = false
  @State private var isPresentingAddAccount: Bool = false
  @State private var isPresentingAddAccountConfirmation: Bool = false
  @State private var savedBSSDestination: BuySendSwapDestination?
  
  @Environment(\.sizeCategory) private var sizeCategory
  /// Reference to the collection view used to back the `List` on iOS 16+
  @State private var collectionViewRef: WeakRef<UICollectionView>?

  @Environment(\.buySendSwapDestination)
  private var buySendSwapDestination: Binding<BuySendSwapDestination?>

  @Environment(\.openURL) private var openWalletURL

  @ViewBuilder private var accountsBalanceView: some View {
    Section(
      header: WalletListHeaderView(title: Text(Strings.Wallet.accountsPageTitle)),
      footer: Button(action: {
        isShowingAddAccount = true
      }) {
        Text(Strings.Wallet.addAccountTitle)
      }
        .listRowInsets(.zero)
        .buttonStyle(BraveOutlineButtonStyle(size: .small))
        .padding(.vertical, 8)
    ) {
      Group {
        if assetDetailStore.accounts.isEmpty {
          Text(Strings.Wallet.noAccounts)
            .redacted(reason: assetDetailStore.isLoadingAccountBalances ? .placeholder : [])
            .shimmer(assetDetailStore.isLoadingAccountBalances)
            .font(.footnote)
        } else {
          ForEach(assetDetailStore.accounts) { viewModel in
            HStack {
              AddressView(address: viewModel.account.address) {
                AccountView(address: viewModel.account.address, name: viewModel.account.name)
              }
              let showFiatPlaceholder = viewModel.fiatBalance.isEmpty && assetDetailStore.isLoadingPrice
              let showBalancePlaceholder = viewModel.balance.isEmpty && assetDetailStore.isLoadingAccountBalances
              if assetDetailStore.assetDetailToken.isNft || assetDetailStore.assetDetailToken.isErc721 {
                Text(showBalancePlaceholder ? "0 \(assetDetailStore.assetDetailToken.symbol)" : "\(viewModel.balance) \(assetDetailStore.assetDetailToken.symbol)")
                  .redacted(reason: showBalancePlaceholder ? .placeholder : [])
                  .shimmer(assetDetailStore.isLoadingAccountBalances)
                  .font(.footnote)
                  .foregroundColor(Color(.secondaryBraveLabel))
              } else {
                VStack(alignment: .trailing) {
                  Text(showFiatPlaceholder ? "$0.00" : viewModel.fiatBalance)
                    .redacted(reason: showFiatPlaceholder ? .placeholder : [])
                    .shimmer(assetDetailStore.isLoadingPrice)
                  Text(showBalancePlaceholder ? "0.0000 \(assetDetailStore.assetDetailToken.symbol)" : "\(viewModel.balance) \(assetDetailStore.assetDetailToken.symbol)")
                    .redacted(reason: showBalancePlaceholder ? .placeholder : [])
                    .shimmer(assetDetailStore.isLoadingAccountBalances)
                }
                .font(.footnote)
                .foregroundColor(Color(.secondaryBraveLabel))
              }
            }
          }
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
  }
  
  @ViewBuilder private var transactionsView: some View {
    Section(
      header: WalletListHeaderView(title: Text(Strings.Wallet.transactionsTitle))
    ) {
      Group {
        if assetDetailStore.transactionSummaries.isEmpty {
          Text(Strings.Wallet.noTransactions)
            .font(.footnote)
        } else {
          ForEach(assetDetailStore.transactionSummaries) { txSummary in
            Button(action: {
              self.transactionDetails = assetDetailStore.transactionDetailsStore(for: txSummary.txInfo)
            }) {
              TransactionSummaryView(summary: txSummary, displayAccountCreator: true)
            }
            .contextMenu {
              if !txSummary.txHash.isEmpty {
                Button(action: {
                  if let txNetwork = self.networkStore.allChains.first(where: { $0.chainId == txSummary.txInfo.chainId }),
                     let url = txNetwork.txBlockExplorerLink(txHash: txSummary.txHash, for: txNetwork.coin) {
                    openWalletURL(url)
                  }
                }) {
                  Label(Strings.Wallet.viewOnBlockExplorer, systemImage: "arrow.up.forward.square")
                }
              }
            }
          }
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
  }
  
  private func coinMarketInfoView(_ coinMarket: BraveWallet.CoinMarket) -> some View {
    Section {
      HStack {
        VStack(spacing: 10) {
          Text("\(coinMarket.marketCapRank)")
            .font(.title3.weight(.semibold))
          Text(Strings.Wallet.coinMarketRank)
            .font(.footnote)
        }
        Spacer()
        VStack(spacing: 10) {
          let computed = assetDetailStore.currencyFormatter.string(from: NSNumber(value: BraveWallet.CoinMarket.abbreviateToBillion(input: coinMarket.totalVolume))) ?? ""
          Text("\(computed)B")
            .font(.title3.weight(.semibold))
          Text(Strings.Wallet.coinMarket24HVolume)
            .font(.footnote)
        }
        Spacer()
        VStack(spacing: 10) {
          let computed = assetDetailStore.currencyFormatter.string(from: NSNumber(value: BraveWallet.CoinMarket.abbreviateToBillion(input: coinMarket.marketCap))) ?? ""
          Text("\(computed)B")
            .font(.title3.weight(.semibold))
          Text(Strings.Wallet.coinMarketMarketCap)
            .font(.footnote)
        }
      }
      .foregroundColor(Color(.braveLabel))
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    } header: {
      Text(Strings.Wallet.coinMarketInformation)
    }
  }
  
  var body: some View {
    List {
      Section(
        header: AssetDetailHeaderView(
          assetDetailStore: assetDetailStore,
          keyringStore: keyringStore,
          networkStore: networkStore,
          buySendSwapDestination: buySendSwapDestination,
          isShowingBridgeAlert: $isShowingAuroraBridgeAlert,
          onAccountCreationNeeded: { savedDestination in
            isPresentingAddAccountConfirmation = true
            savedBSSDestination = savedDestination
          }
        )
        .resetListHeaderStyle()
        .padding(.horizontal, tableInset)  // inset grouped layout margins workaround
      ) {
      }
      switch assetDetailStore.assetDetailType {
      case .blockchainToken(_):
        accountsBalanceView
        transactionsView
      case .coinMarket(let coinMarket):
        coinMarketInfoView(coinMarket)
      }
      if !assetDetailStore.assetDetailToken.isNft && !assetDetailStore.assetDetailToken.isErc721 {
        Section {
          EmptyView()
        } header: {
          Text(Strings.Wallet.coinGeckoDisclaimer)
            .multilineTextAlignment(.center)
            .font(.footnote)
            .foregroundColor(Color(.secondaryBraveLabel))
            .frame(maxWidth: .infinity)
            .listRowBackground(Color(.braveGroupedBackground))
            .resetListHeaderStyle(insets: nil)
        }
      }
    }
    .listStyle(InsetGroupedListStyle())
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .navigationTitle(assetDetailStore.assetDetailToken.name)
    .navigationBarTitleDisplayMode(.inline)
    .onAppear {
      assetDetailStore.update()
    }
    .introspectTableView { tableView in
      tableInset = -tableView.layoutMargins.left
    }
    .onChange(of: sizeCategory) { _ in
      // Fix broken header when text size changes on iOS 16+
      self.collectionViewRef?.value?.collectionViewLayout.invalidateLayout()
    }
    .introspect(
      selector: TargetViewSelector.ancestorOrSiblingContaining
    ) { (collectionView: UICollectionView) in
      self.collectionViewRef = .init(collectionView)
    }
    .background(
      Color.clear
        .sheet(isPresented: $isShowingAddAccount) {
          NavigationView {
            AddAccountView(
              keyringStore: keyringStore,
              networkStore: networkStore
            )
          }
          .navigationViewStyle(StackNavigationViewStyle())
        }
    )
    .background(
      Color.clear
        .sheet(
          isPresented: Binding(
            get: { self.transactionDetails != nil },
            set: { if !$0 { self.transactionDetails = nil } }
          )
        ) {
          if let transactionDetailsStore = transactionDetails {
            TransactionDetailsView(
              transactionDetailsStore: transactionDetailsStore,
              networkStore: networkStore
            )
          }
        }
    )
    .background(
      WalletPromptView(
        isPresented: $isShowingAuroraBridgeAlert,
        primaryButton: .init(
          title: Strings.Wallet.auroraBridgeButtonTitle,
          action: { _ in
            isShowingAuroraBridgeAlert = false
            if let link = WalletConstants.auroraBridgeLink {
              openWalletURL(link)
            }
          }
        ),
        showCloseButton: false,
        content: {
          VStack(spacing: 10) {
            Text(Strings.Wallet.auroraBridgeAlertTitle)
              .font(.headline.weight(.bold))
              .multilineTextAlignment(.center)
              .padding(.vertical)
            Text(Strings.Wallet.auroraBridgeAlertDescription)
              .multilineTextAlignment(.center)
              .font(.subheadline)
          }
        },
        footer: {
          VStack(spacing: 8) {
            Button(action: {
              isShowingAuroraBridgeAlert = false
              Preferences.Wallet.showAuroraPopup.value = false
            }) {
              Text(Strings.Wallet.auroraPopupDontShowAgain)
                .foregroundColor(Color(.braveLabel))
                .font(.callout.weight(.semibold))
            }
            Button {
              isShowingAuroraBridgeAlert = false
              if let link = WalletConstants.auroraBridgeOverviewLink {
                openWalletURL(link)
              }
            } label: {
              Text(Strings.Wallet.auroraBridgeLearnMore)
                .multilineTextAlignment(.center)
                .foregroundColor(Color(.braveBlurpleTint))
                .font(.subheadline)
            }
            Button {
              isShowingAuroraBridgeAlert = false
              if let link = WalletConstants.auroraBridgeRiskLink {
                openWalletURL(link)
              }
            } label: {
              Text(Strings.Wallet.auroraBridgeRisk)
                .multilineTextAlignment(.center)
                .foregroundColor(Color(.braveBlurpleTint))
                .font(.subheadline)
            }
          }
          .padding(.top, 16)
        }
      )
    )
    .onChange(of: keyringStore.defaultKeyring) { newValue in
      if newValue.isLocked, isShowingAuroraBridgeAlert {
        isShowingAuroraBridgeAlert = false
      }
    }
    .addAccount(
      keyringStore: keyringStore,
      networkStore: networkStore,
      accountNetwork: networkStore.network(for: assetDetailStore.assetDetailToken),
      isShowingConfirmation: $isPresentingAddAccountConfirmation,
      isShowingAddAccount: $isPresentingAddAccount,
      onConfirmAddAccount: { isPresentingAddAccount = true },
      onCancelAddAccount: nil,
      onAddAccountDismissed: {
        Task { @MainActor in
          if await assetDetailStore.handleDismissAddAccount() {
            if let savedBSSDestination {
              buySendSwapDestination.wrappedValue = savedBSSDestination
              self.savedBSSDestination = nil
            }
          }
        }
      }
    )
  }
}

#if DEBUG
struct CurrencyDetailView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      AssetDetailView(
        assetDetailStore: .previewStore,
        keyringStore: .previewStore,
        networkStore: .previewStore
      )
      .navigationBarTitleDisplayMode(.inline)
    }
    .previewColorSchemes()
  }
}
#endif
