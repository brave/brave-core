// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import DesignSystem
import Foundation
import Introspect
import Preferences
import Shared
import Strings
import SwiftUI
import UIKit

struct AssetDetailView: View {
  @ObservedObject var assetDetailStore: AssetDetailStore
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore

  @State private var tableInset: CGFloat = -16.0
  @State private var transactionDetails: TransactionDetailsStore?
  @State private var isShowingAuroraBridgeAlert: Bool = false
  @State private var isPresentingAddAccount: Bool = false
  @State private var isPresentingAddAccountConfirmation: Bool = false
  @State private var savedWalletActionestination: WalletActionDestination?
  @State private var isShowingMoreActionSheet: Bool = false

  @Environment(\.sizeCategory) private var sizeCategory
  /// Reference to the collection view used to back the `List` on iOS 16+
  @State private var collectionViewRef: WeakRef<UICollectionView>?

  @Environment(\.walletActionDestination)
  @Binding private var walletActionDestination: WalletActionDestination?

  @Environment(\.openURL) private var openWalletURL
  @ObservedObject private var isShowingBalances = Preferences.Wallet.isShowingBalances

  @State private var selectedContent: AssetDetailSegmentedControl.Item = .accounts
  /// Query displayed in the search bar above the transactions.
  @State private var query: String = ""
  /// Error message displayed in alert for transaction list
  @State private var errorMessage: String?
  @State private var isDoNotShowCheckboxChecked: Bool = false

  private var accountsBalanceHeader: some View {
    VStack(spacing: 8) {
      HStack {
        Text(Strings.Wallet.accountsPageTitle)
          .foregroundColor(Color(braveSystemName: .textPrimary))
          .font(.title3.weight(.semibold))
          .frame(maxWidth: .infinity, alignment: .leading)

        Spacer()

        if assetDetailStore.isLoadingAccountBalances {
          Text(Strings.Wallet.totalBalance)
            .redacted(reason: assetDetailStore.isLoadingAccountBalances ? .placeholder : [])
            .shimmer(assetDetailStore.isLoadingAccountBalances)
        } else {
          Text("\(assetDetailStore.totalBalance) \(assetDetailStore.assetDetailToken.symbol)")
            .foregroundColor(Color(.braveLabel))
        }
      }

      DividerLine()
    }
  }

  private func accontBalanceRow(_ viewModel: AccountAssetViewModel) -> some View {
    HStack {
      AddressView(address: viewModel.account.address) {
        AccountView(account: viewModel.account)
      }
      let showFiatPlaceholder = viewModel.fiatBalance.isEmpty && assetDetailStore.isLoadingPrice
      let showBalancePlaceholder =
        viewModel.balance.isEmpty && assetDetailStore.isLoadingAccountBalances
      Group {
        if isShowingBalances.value {
          VStack(alignment: .trailing) {
            Text(
              showBalancePlaceholder
                ? "0.0000 \(assetDetailStore.assetDetailToken.symbol)"
                : "\(viewModel.balance) \(assetDetailStore.assetDetailToken.symbol)"
            )
            .font(.subheadline.weight(.semibold))
            .foregroundColor(Color(.bravePrimary))
            .redacted(reason: showBalancePlaceholder ? .placeholder : [])
            .shimmer(assetDetailStore.isLoadingAccountBalances)
            Text(showFiatPlaceholder ? "$0.00" : viewModel.fiatBalance)
              .font(.footnote)
              .foregroundColor(Color(.braveLabel))
              .redacted(reason: showFiatPlaceholder ? .placeholder : [])
              .shimmer(assetDetailStore.isLoadingPrice)
          }
        } else {
          Text("****")
        }
      }
      .font(.footnote)
      .foregroundColor(Color(.secondaryBraveLabel))
    }
  }

  @ViewBuilder private var accountsBalanceView: some View {
    VStack {
      if assetDetailStore.nonZeroBalanceAccounts.isEmpty {
        emptyAccountState
      } else {
        accountsBalanceHeader

        ForEach(assetDetailStore.nonZeroBalanceAccounts) { viewModel in
          accontBalanceRow(viewModel)
        }
      }
    }
  }

  struct CoinMarketInfo: Identifiable {
    let title: String
    let value: String
    var id: String { title }
  }

  @ViewBuilder private func coinMarketInfoView(_ coinMarket: BraveWallet.CoinMarket) -> some View {
    VStack(spacing: 16) {
      HStack(alignment: .top, spacing: 40) {
        if case .coinMarket(let coinMarket) = assetDetailStore.assetDetailType,
          assetDetailStore.convertCoinMarketToDepositableToken(
            symbol: coinMarket.symbol
          ) != nil
        {
          PortfolioHeaderButton(style: .deposit) {
            let destination = WalletActionDestination(
              kind: .deposit(query: coinMarket.symbol)
            )
            if assetDetailStore.allAccountsForToken.isEmpty {
              onAccountCreationNeeded(destination)
            } else {
              walletActionDestination = destination
            }
          }
        }
      }
      .padding(.horizontal, 16)
      VStack(spacing: 12) {
        Text(Strings.Wallet.coinMarketInformation)
          .foregroundColor(Color(braveSystemName: .textPrimary))
          .font(.title3.weight(.semibold))
          .frame(maxWidth: .infinity, alignment: .leading)

        DividerLine()
      }
      let grids = [GridItem(.adaptive(minimum: 160), spacing: 8, alignment: .top)]
      let info: [CoinMarketInfo] = {
        let computedMarketCap =
          assetDetailStore.currencyFormatter.formatAsFiat(
            BraveWallet.CoinMarket.abbreviateToBillion(input: coinMarket.marketCap)
          ) ?? ""
        let computedTotalVolume =
          assetDetailStore.currencyFormatter.formatAsFiat(
            BraveWallet.CoinMarket.abbreviateToBillion(input: coinMarket.totalVolume)
          ) ?? ""
        return [
          .init(title: Strings.Wallet.coinMarketRank, value: "#\(coinMarket.marketCapRank)"),
          .init(title: Strings.Wallet.coinMarketMarketCap, value: "\(computedMarketCap)B"),
          .init(title: Strings.Wallet.coinMarket24HVolume, value: "\(computedTotalVolume)B"),
        ]
      }()
      LazyVGrid(columns: grids) {
        ForEach(info) { item in
          VStack(spacing: 4) {
            Text(item.value)
              .font(.body.weight(.semibold))
              .foregroundColor(Color(.bravePrimary))
            Text(item.title)
              .font(.caption2)
              .foregroundColor(Color(.braveLabel))
          }
          .frame(maxWidth: .infinity)
          .padding(.vertical, 16)
          .overlay {
            RoundedRectangle(cornerRadius: 8)
              .stroke(Color(braveSystemName: .dividerSubtle), lineWidth: 1)
          }
        }
      }
    }
    .padding(.horizontal)
  }

  private var emptyTransactionState: some View {
    VStack(spacing: 10) {
      Image("transaction-empty", bundle: .module)
        .aspectRatio(contentMode: .fit)
      Text(Strings.Wallet.noTransactions)
        .font(.headline)
        .foregroundColor(Color(WalletV2Design.textPrimary))
      Text(Strings.Wallet.activityPageEmptyDescription)
        .font(.footnote)
        .foregroundColor(Color(WalletV2Design.textSecondary))
    }
    .multilineTextAlignment(.center)
    .padding(.vertical)
  }

  private var emptyAccountState: some View {
    VStack(spacing: 10) {
      Image("account-empty", bundle: .module)
        .aspectRatio(contentMode: .fit)
      Text(Strings.Wallet.noAccounts)
        .font(.headline)
        .foregroundColor(Color(WalletV2Design.textPrimary))
      Text(Strings.Wallet.noAccountDescription)
        .font(.footnote)
        .foregroundColor(Color(WalletV2Design.textSecondary))
    }
    .multilineTextAlignment(.center)
    .padding(.vertical)
  }

  @ViewBuilder var actionButtonsContainer: some View {
    HStack(alignment: .top, spacing: 40) {
      if assetDetailStore.isBuySupported {
        PortfolioHeaderButton(style: .buy) {
          let destination = WalletActionDestination(
            kind: .buy,
            initialToken: assetDetailStore.assetDetailToken
          )
          if assetDetailStore.allAccountsForToken.isEmpty {
            onAccountCreationNeeded(destination)
          } else {
            walletActionDestination = destination
          }
        }
      }
      if assetDetailStore.isSendSupported {
        PortfolioHeaderButton(style: .send) {
          let destination = WalletActionDestination(
            kind: .send,
            initialToken: assetDetailStore.assetDetailToken
          )
          if assetDetailStore.allAccountsForToken.isEmpty {
            onAccountCreationNeeded(destination)
          } else {
            walletActionDestination = destination
          }
        }
      }
      if assetDetailStore.isSwapSupported {
        PortfolioHeaderButton(style: .swap) {
          let destination = WalletActionDestination(
            kind: .swap,
            initialToken: assetDetailStore.assetDetailToken
          )
          if assetDetailStore.allAccountsForToken.isEmpty {
            onAccountCreationNeeded(destination)
          } else {
            walletActionDestination = destination
          }
        }
      }
      if case .blockchainToken(let token) = assetDetailStore.assetDetailType {
        if token.isAuroraSupportedToken {
          PortfolioHeaderButton(style: .more) {
            isShowingMoreActionSheet = true
          }
        } else {
          PortfolioHeaderButton(style: .deposit) {
            let destination = WalletActionDestination(
              kind: .deposit(query: nil),
              initialToken: assetDetailStore.assetDetailToken
            )
            if assetDetailStore.allAccountsForToken.isEmpty {
              onAccountCreationNeeded(destination)
            } else {
              walletActionDestination = destination
            }
          }
        }
      }
    }
    .padding(.horizontal, 16)
    .transaction { transaction in
      transaction.animation = nil
      transaction.disablesAnimations = true
    }
  }

  @ViewBuilder private var tokenContentContainer: some View {
    VStack(spacing: 0) {
      actionButtonsContainer
        .padding(.bottom, 40)

      AssetDetailSegmentedControl(selected: $selectedContent)
        .padding(.horizontal)
      if selectedContent == .accounts {
        accountsBalanceView
          .padding(.horizontal)
          .padding(.top, 16)
      } else {
        if assetDetailStore.transactionSections.isEmpty {
          emptyTransactionState
        } else {
          TransactionsListView(
            transactionSections: assetDetailStore.transactionSections,
            query: $query,
            errorMessage: $errorMessage,
            showFilter: false,
            filtersButtonTapped: {},
            transactionTapped: { tx in
              self.transactionDetails = assetDetailStore.transactionDetailsStore(for: tx)
            },
            transactionFollowUpActionTapped: { action, tx in
              Task { @MainActor in
                guard
                  let errorMessage = await assetDetailStore.handleTransactionFollowUpAction(
                    action,
                    transaction: tx
                  )
                else { return }
                self.errorMessage = errorMessage
              }
            }
          )
        }
      }
    }
  }

  private var assetDetailContentView: some View {
    LazyVStack {
      switch assetDetailStore.assetDetailType {
      case .blockchainToken(_):
        tokenContentContainer
          .padding(.bottom, 12)

        if (selectedContent == .accounts && !assetDetailStore.nonZeroBalanceAccounts.isEmpty)
          || (selectedContent == .transactions && !assetDetailStore.transactionSections.isEmpty)
        {
          Text(Strings.Wallet.coinGeckoDisclaimer)
            .multilineTextAlignment(.center)
            .font(.footnote)
            .foregroundColor(Color(.secondaryBraveLabel))
            .frame(maxWidth: .infinity)
            .padding(.bottom, 12)
        }
      case .coinMarket(let coinMarket):
        coinMarketInfoView(coinMarket)
          .padding(.bottom, 20)
        Text(Strings.Wallet.coinGeckoDisclaimer)
          .multilineTextAlignment(.center)
          .font(.footnote)
          .foregroundColor(Color(.secondaryBraveLabel))
          .frame(maxWidth: .infinity)
          .padding(.bottom, 12)
      }
    }
    .padding(.vertical)
    .background(Color(braveSystemName: .containerBackground))
  }

  var body: some View {
    ScrollView {
      VStack(spacing: 0) {
        AssetDetailHeaderView(
          assetDetailStore: assetDetailStore,
          keyringStore: keyringStore,
          networkStore: networkStore
        )

        assetDetailContentView
      }
    }
    .background(
      Color(braveSystemName: .containerBackground).edgesIgnoringSafeArea(.all)
    )
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
    .actionSheet(isPresented: $isShowingMoreActionSheet) {
      ActionSheet(
        title: Text(
          "\(assetDetailStore.assetDetailToken.name) (\(assetDetailStore.assetDetailToken.symbol))"
        ),
        buttons: [
          .cancel(),
          .default(
            Text(Strings.Wallet.deposit),
            action: {
              let destination = WalletActionDestination(
                kind: .deposit(query: nil),
                initialToken: assetDetailStore.assetDetailToken
              )
              if assetDetailStore.allAccountsForToken.isEmpty {
                onAccountCreationNeeded(destination)
              } else {
                walletActionDestination = destination
              }
            }
          ),
          .default(
            Text(Strings.Wallet.auroraBridgeButtonTitle),
            action: {
              if Preferences.Wallet.showAuroraPopup.value {
                isShowingAuroraBridgeAlert = true
              } else {
                if let link = WalletConstants.auroraBridgeLink {
                  openWalletURL(link)
                }
              }
            }
          ),
        ]
      )
    }
    .background(
      Color.clear
        .sheet(
          isPresented: Binding(
            get: { self.transactionDetails != nil },
            set: {
              if !$0 {
                self.transactionDetails = nil
                self.assetDetailStore.closeTransactionDetailsStore()
              }
            }
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
            if isDoNotShowCheckboxChecked {
              Preferences.Wallet.showAuroraPopup.value = false
            }
            isShowingAuroraBridgeAlert = false
            if let link = WalletConstants.auroraBridgeLink {
              openWalletURL(link)
            }
          }
        ),
        secondaryButton: .init(
          title: Strings.CancelString,
          action: { _ in
            if isDoNotShowCheckboxChecked {
              Preferences.Wallet.showAuroraPopup.value = false
            }
            isShowingAuroraBridgeAlert = false
          }
        ),
        showCloseButton: false,
        content: {
          VStack(spacing: 16) {
            Text(Strings.Wallet.auroraBridgeAlertTitle)
              .font(.body.weight(.medium))
              .foregroundColor(Color(.bravePrimary))
              .multilineTextAlignment(.center)
            Text(Strings.Wallet.auroraBridgeAlertDescription)
              .multilineTextAlignment(.center)
              .font(.footnote)
              .foregroundColor(Color(.braveLabel))
            Button {
              isShowingAuroraBridgeAlert = false
              if let link = WalletConstants.auroraBridgeOverviewLink {
                openWalletURL(link)
              }
            } label: {
              Text(Strings.Wallet.auroraBridgeLearnMore)
                .multilineTextAlignment(.center)
                .foregroundColor(Color(.braveBlurpleTint))
                .font(.footnote.weight(.semibold))
                .padding(8)
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
                .font(.footnote.weight(.semibold))
                .padding(8)
            }
            HStack(alignment: .top, spacing: 8) {
              LegalCheckbox(isChecked: $isDoNotShowCheckboxChecked)
              Text(Strings.Wallet.auroraPopupDontShowAgain)
                .foregroundColor(Color(.bravePrimary))
                .font(.subheadline)
              Spacer()
            }
          }
          .padding(.bottom, 16)
        }
      )
    )
    .onChange(of: keyringStore.isWalletLocked) { isLocked in
      if isLocked, isShowingAuroraBridgeAlert {
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
            if let savedWalletActionestination {
              walletActionDestination = savedWalletActionestination
              self.savedWalletActionestination = nil
            }
          }
        }
      }
    )
  }

  private func onAccountCreationNeeded(_ destination: WalletActionDestination) {
    isPresentingAddAccountConfirmation = true
    savedWalletActionestination = destination
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

struct AssetDetailSegmentedControl: View {

  enum Item: Int, Equatable, CaseIterable, Identifiable, WalletSegmentedControlItem {
    case accounts
    case transactions

    var title: String {
      switch self {
      case .accounts: return Strings.Wallet.accountsPageTitle
      case .transactions: return Strings.Wallet.transactionsTitle
      }
    }

    var id: Int { rawValue }
  }

  @Binding var selected: Item

  var body: some View {
    WalletSegmentedControl(
      items: Item.allCases,
      selected: $selected
    )
  }
}
