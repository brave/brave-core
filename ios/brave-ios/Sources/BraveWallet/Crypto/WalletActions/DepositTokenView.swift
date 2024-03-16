// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import DesignSystem
import Strings
import SwiftUI

struct DepositTokenView: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var depositTokenStore: DepositTokenStore

  var prefilledQuery: String?
  var onDismiss: () -> Void

  @State private var isPresentingNetworkFilter = false
  @State private var selectedAccount: BraveWallet.AccountInfo?
  // Assignment happens when user selects a token and user's wallet
  // has an account is the same coin type as the selected token
  @State private var selectedTokenViewModel: DepositTokenViewModel?
  // Assignment happens when user selects a token and user's wallet
  // has no account is the same coin type as the selected token
  @State private var savedTokenViewModel: DepositTokenViewModel?
  @State private var isPresentingAddAccount: Bool = false
  @State private var isPresentingAddAccountConfirmation: Bool = false

  private var availableAccounts: [BraveWallet.AccountInfo] {
    guard let token = depositTokenStore.prefilledToken else { return [] }
    return depositTokenStore.allAccounts.filter({ $0.coin == token.coin })
  }

  var body: some View {
    NavigationView {
      Group {
        if let prefilledAccount = depositTokenStore.prefilledAccount {
          DepositDetailsView(
            type: .prefilledAccount(account: prefilledAccount),
            allNetworks: depositTokenStore.allNetworks
          )
        } else if let prefilledToken = depositTokenStore.prefilledToken, !availableAccounts.isEmpty
        {
          DepositDetailsView(
            type: .prefilledToken(token: prefilledToken, availableAccounts: availableAccounts),
            allNetworks: depositTokenStore.allNetworks
          )
        } else {
          TokenList(
            tokens: depositTokenStore.assetViewModels,
            prefilledQuery: prefilledQuery
          ) { query, viewModel in
            let symbolMatch = viewModel.token.symbol.localizedCaseInsensitiveContains(query)
            let nameMatch = viewModel.token.name.localizedCaseInsensitiveContains(query)
            return symbolMatch || nameMatch
          } header: {
            TokenListHeaderView(title: Strings.Wallet.assetsTitle)
          } content: { viewModel in
            Button {
              if depositTokenStore.allAccounts.contains(where: { $0.coin == viewModel.token.coin })
              {
                selectedTokenViewModel = viewModel
              } else {
                savedTokenViewModel = viewModel
                isPresentingAddAccountConfirmation = true
              }
            } label: {
              HStack {
                AssetIconView(
                  token: viewModel.token,
                  network: viewModel.network,
                  shouldShowNetworkIcon: true
                )
                VStack(alignment: .leading) {
                  Text(viewModel.token.name)
                    .font(.footnote)
                    .fontWeight(.semibold)
                    .foregroundColor(Color(.bravePrimary))
                  Text(
                    String.localizedStringWithFormat(
                      Strings.Wallet.userAssetSymbolNetworkDesc,
                      viewModel.token.symbol,
                      viewModel.network.chainName
                    )
                  )
                  .font(.caption)
                  .foregroundColor(Color(.braveLabel))
                }
                Spacer()
                Image(systemName: "chevron.right")
                  .font(.body.weight(.semibold))
                  .foregroundColor(Color(.separator))
              }
            }
            .padding(.vertical, 6)
            .accessibilityElement()
            .accessibilityLabel(accessibilityLabel(viewModel))
          }
          .toolbar {
            ToolbarItemGroup(placement: .bottomBar) {
              Button {
                self.isPresentingNetworkFilter = true
              } label: {
                Image(braveSystemName: "leo.tune")
                  .font(.footnote.weight(.medium))
                  .foregroundColor(Color(.braveBlurpleTint))
                  .clipShape(Rectangle())
              }
              Spacer()
            }
          }
        }
      }
      .navigationViewStyle(StackNavigationViewStyle())
      .navigationTitle(Strings.Wallet.deposit)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItemGroup(placement: .cancellationAction) {
          Button {
            onDismiss()
          } label: {
            Text(Strings.cancelButtonTitle)
              .foregroundColor(Color(.braveBlurpleTint))
          }
        }
      }
      .sheet(isPresented: $isPresentingNetworkFilter) {
        NavigationView {
          NetworkFilterView(
            networks: depositTokenStore.networkFilters,
            networkStore: networkStore,
            saveAction: { selectedNetworks in
              depositTokenStore.networkFilters = selectedNetworks
            }
          )
        }
        .navigationViewStyle(.stack)
        .onDisappear {
          networkStore.closeNetworkSelectionStore()
        }
      }
      .background(
        NavigationLink(
          isActive: Binding(
            get: { selectedTokenViewModel != nil },
            set: { if !$0 { selectedTokenViewModel = nil } }
          ),
          destination: {
            if let selectedTokenViewModel {
              DepositDetailsView(
                type: .prefilledToken(
                  token: selectedTokenViewModel.token,
                  availableAccounts: depositTokenStore.allAccounts.filter {
                    $0.coin == selectedTokenViewModel.token.coin
                  }
                ),
                allNetworks: depositTokenStore.allNetworks
              )
            }
          },
          label: {
            EmptyView()
          }
        )
      )
    }
    .task {
      depositTokenStore.setup()
    }
    .addAccount(
      keyringStore: keyringStore,
      networkStore: networkStore,
      accountNetwork: savedTokenViewModel?.network,
      isShowingConfirmation: $isPresentingAddAccountConfirmation,
      isShowingAddAccount: $isPresentingAddAccount,
      onConfirmAddAccount: { isPresentingAddAccount = true },
      onCancelAddAccount: nil,
      onAddAccountDismissed: {
        Task { @MainActor in
          guard let savedTokenViewModel else { return }
          if await self.depositTokenStore.handleDismissAddAccount(savedTokenViewModel) {
            self.selectedTokenViewModel = savedTokenViewModel
            self.savedTokenViewModel = nil
          }
        }
      }
    )
  }

  private func accessibilityLabel(_ viewModel: DepositTokenViewModel) -> String {
    "\(viewModel.token.name), \(viewModel.token.symbol), \(viewModel.network.chainName)"
  }
}

private struct DepositDetailsView: View {
  var type: DepositType
  var allNetworks: [BraveWallet.NetworkInfo]

  @State private var selectedAccount: BraveWallet.AccountInfo?
  @State private var isPresentingAccountPicker: Bool = false
  @State private var addressToShare: String?
  @ScaledMetric private var avatarSize = 24.0

  enum DepositType {
    case prefilledAccount(account: BraveWallet.AccountInfo)
    case prefilledToken(
      token: BraveWallet.BlockchainToken,
      availableAccounts: [BraveWallet.AccountInfo]
    )
  }

  init(
    type: DepositType,
    allNetworks: [BraveWallet.NetworkInfo]
  ) {
    self.type = type
    self.allNetworks = allNetworks
    if case .prefilledToken(_, let availableAccounts) = type {
      self._selectedAccount = State(initialValue: availableAccounts.first)
    }
  }

  private var accountPicker: some View {
    HStack(spacing: 16) {
      Blockie(address: selectedAccount?.address ?? "")
        .frame(width: avatarSize, height: avatarSize)
      Text(selectedAccount?.name ?? "")
        .fontWeight(.semibold)
        .foregroundColor(Color(.bravePrimary))
        .multilineTextAlignment(.leading)
        .font(.body)
      Image(braveSystemName: "leo.arrow.small-down")
        .font(.callout.weight(.medium))
        .foregroundColor(Color(.primaryButtonTint))
        .padding(.leading, 48)
    }
    .padding(.horizontal, 12)
    .padding(.vertical, 8)
    .background {
      RoundedRectangle(cornerRadius: 8)
        .fill(Color(.secondaryBraveBackground))
    }
  }

  @ViewBuilder private func depositHeader(
    coin: BraveWallet.CoinType,
    networks: [BraveWallet.NetworkInfo]
  ) -> some View {
    VStack(spacing: 8) {
      switch coin {
      case .eth:
        Text(Strings.Wallet.ethAccountDescription)
          .fontWeight(.semibold)
          .foregroundColor(Color(.bravePrimary))
        MultipleNetworkIconsView(networks: networks.filter({ $0.coin == .eth }), maxIcons: 8)
      case .sol:
        Text(Strings.Wallet.solAccountDescription)
          .fontWeight(.semibold)
          .foregroundColor(Color(.bravePrimary))
        MultipleNetworkIconsView(networks: networks.filter({ $0.coin == .sol }))
      case .fil:
        Text(Strings.Wallet.filAccountDescription)
          .fontWeight(.semibold)
          .foregroundColor(Color(.bravePrimary))
        MultipleNetworkIconsView(networks: networks.filter({ $0.coin == .fil }))
      case .zec, .btc:
        EmptyView()
      @unknown default:
        EmptyView()
      }
    }
  }

  @ViewBuilder private func qrCodeView(_ account: BraveWallet.AccountInfo) -> some View {
    VStack(spacing: 12) {
      RoundedRectangle(cornerRadius: 20, style: .continuous)
        .stroke(Color(.secondaryButtonTint).opacity(0.5), lineWidth: 1)
        .frame(width: 184, height: 184)
        .overlay(
          Group {
            if let image = account.qrCodeImage?.cgImage {
              Image(uiImage: UIImage(cgImage: image))
                .resizable()
                .interpolation(.none)
                .scaledToFit()
                .padding()
                .accessibilityHidden(true)
            }
          }
        )
        .padding(.bottom, 12)
      Text(account.name)
        .font(.callout.weight(.semibold))
        .multilineTextAlignment(.center)
      Text(account.address)
        .font(.subheadline)
        .foregroundColor(Color(.secondaryBraveLabel))
        .multilineTextAlignment(.center)
      HStack {
        Button {
          UIPasteboard.general.string = account.address
        } label: {
          HStack {
            Image(braveSystemName: "leo.copy")
            Text(Strings.Wallet.depositAddressCopy)
              .font(.footnote.weight(.semibold))
          }
          .foregroundColor(Color(.braveBlurpleTint))
        }
        .buttonStyle(BraveOutlineButtonStyle(size: .normal))
        Button {
          addressToShare = account.address
        } label: {
          HStack {
            Image(braveSystemName: "leo.share.macos")
            Text(Strings.share)
              .font(.footnote.weight(.semibold))
          }
          .foregroundColor(Color(.braveBlurpleTint))
        }
        .buttonStyle(BraveOutlineButtonStyle(size: .normal))
      }
      .padding(.top, 12)
    }
  }

  private var ethNetworksCombined: String {
    let networks =
      allNetworks
      .filter {
        $0.coin == .eth && $0.chainId.lowercased() != BraveWallet.MainnetChainId.lowercased()
      }
      .map { $0.chainName }
    return networks.joined(separator: ", ")
  }

  @ViewBuilder private var ethDisclosureView: some View {
    Text(String.localizedStringWithFormat(Strings.Wallet.depositEthDisclosure, ethNetworksCombined))
      .font(.caption2)
      .foregroundColor(Color(.secondaryBraveLabel))
      .multilineTextAlignment(.center)
  }

  var body: some View {
    ScrollView {
      VStack(spacing: 24) {
        switch type {
        case .prefilledAccount(let account):
          depositHeader(
            coin: account.coin,
            networks: allNetworks
          )
          qrCodeView(account)
          if account.coin == .eth {
            ethDisclosureView
          }
        case .prefilledToken(let token, _):
          Button {
            isPresentingAccountPicker = true
          } label: {
            accountPicker
          }
          depositHeader(
            coin: token.coin,
            networks: allNetworks
          )
          if let selectedAccount {
            qrCodeView(selectedAccount)
          }
          if token.coin == .eth {
            ethDisclosureView
          }
        }
      }
      .padding(.vertical, 32)
      .padding(.horizontal, 16)
      .sheet(isPresented: $isPresentingAccountPicker) {
        if let selectedAccount, case .prefilledToken(_, let availableAccounts) = type {
          NavigationView {
            AccountSelectionRootView(
              navigationTitle: Strings.Wallet.selectAccountTitle,
              allAccounts: availableAccounts,
              selectedAccounts: [selectedAccount],
              showsSelectAllButton: false
            ) { selectedAccount in
              self.selectedAccount = selectedAccount
              self.isPresentingAccountPicker = false
            }
          }
          .navigationViewStyle(.stack)
        }
      }
      .sheet(
        isPresented: Binding(
          get: { addressToShare != nil },
          set: { if !$0 { addressToShare = nil } }
        )
      ) {
        if let addressToShare {
          ShareSheetView(
            activityItems: [addressToShare],
            applicationActivities: nil,
            excludedActivityTypes: nil,
            callback: nil
          )
        }
      }
    }
  }
}

#if DEBUG
#Preview {
  DepositTokenView(
    keyringStore: .previewStore,
    networkStore: .previewStore,
    depositTokenStore: .previewStore,
    onDismiss: {}
  )
}
#endif
