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

  private func availableAccounts(
    for token: BraveWallet.BlockchainToken
  ) -> [BraveWallet.AccountInfo] {
    let keyringIdForToken: BraveWallet.KeyringId = .keyringId(
      for: token.coin,
      on: token.chainId
    )
    guard
      let tokenNetwork = depositTokenStore.allNetworks.first(where: {
        $0.supportedKeyrings.contains(keyringIdForToken.rawValue as NSNumber)
      })
    else { return [] }
    return depositTokenStore.allAccounts.accountsFor(network: tokenNetwork)
  }

  var body: some View {
    NavigationView {
      Group {
        if let prefilledAccount = depositTokenStore.prefilledAccount {
          DepositDetailsView(
            type: .prefilledAccount(
              account: prefilledAccount,
              bitcoinAccounts: depositTokenStore.bitcoinAccounts
            ),
            supportedNetworks: depositTokenStore.allNetworks.supportedNetworks(
              keyringId: prefilledAccount.keyringId
            )
          )
        } else if let prefilledToken = depositTokenStore.prefilledToken,
          !availableAccounts(for: prefilledToken).isEmpty
        {
          DepositDetailsView(
            type: .prefilledToken(
              token: prefilledToken,
              availableAccounts: availableAccounts(for: prefilledToken),
              bitcoinAccounts: depositTokenStore.bitcoinAccounts
            ),
            supportedNetworks: depositTokenStore.allNetworks.supportedNetworks(
              keyringId: .keyringId(
                for: prefilledToken.coin,
                on: prefilledToken.chainId
              )
            )
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
              let keyringIdForToken: BraveWallet.KeyringId = .keyringId(
                for: viewModel.token.coin,
                on: viewModel.token.chainId
              )
              guard
                let tokenNetwork = depositTokenStore.allNetworks.first(where: {
                  $0.supportedKeyrings.contains(keyringIdForToken.rawValue as NSNumber)
                })
              else { return }
              if !depositTokenStore.allAccounts.accountsFor(network: tokenNetwork).isEmpty {
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
                  availableAccounts: availableAccounts(for: selectedTokenViewModel.token),
                  bitcoinAccounts: depositTokenStore.bitcoinAccounts
                ),
                supportedNetworks: depositTokenStore.allNetworks.supportedNetworks(
                  keyringId: .keyringId(
                    for: selectedTokenViewModel.token.coin,
                    on: selectedTokenViewModel.token.chainId
                  )
                )
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
  var supportedNetworks: [BraveWallet.NetworkInfo]

  @State private var selectedAccount: BraveWallet.AccountInfo?
  @State private var isPresentingAccountPicker: Bool = false
  @State private var addressToShare: String?
  @ScaledMetric private var avatarSize = 24.0

  enum DepositType {
    case prefilledAccount(
      account: BraveWallet.AccountInfo,
      bitcoinAccounts: [String: BraveWallet.BitcoinAccountInfo]
    )
    case prefilledToken(
      token: BraveWallet.BlockchainToken,
      availableAccounts: [BraveWallet.AccountInfo],
      bitcoinAccounts: [String: BraveWallet.BitcoinAccountInfo]
    )
  }

  init(
    type: DepositType,
    supportedNetworks: [BraveWallet.NetworkInfo]
  ) {
    self.type = type
    self.supportedNetworks = supportedNetworks
    if case .prefilledToken(_, let availableAccounts, _) = type {
      self._selectedAccount = State(initialValue: availableAccounts.first)
    }
  }

  private var accountPicker: some View {
    HStack(spacing: 16) {
      Blockie(address: selectedAccount?.blockieSeed ?? "")
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
        MultipleNetworkIconsView(networks: supportedNetworks, maxIcons: 8)
      case .sol:
        Text(Strings.Wallet.solAccountDescription)
          .fontWeight(.semibold)
          .foregroundColor(Color(.bravePrimary))
        MultipleNetworkIconsView(networks: supportedNetworks)
      case .fil:
        Text(Strings.Wallet.filAccountDescription)
          .fontWeight(.semibold)
          .foregroundColor(Color(.bravePrimary))
        MultipleNetworkIconsView(networks: supportedNetworks)
      case .btc:
        Text(Strings.Wallet.btcAccountDescription)
          .fontWeight(.semibold)
          .foregroundColor(Color(.bravePrimary))
        MultipleNetworkIconsView(networks: networks.filter({ $0.coin == .btc }))
      case .zec:
        EmptyView()
      @unknown default:
        EmptyView()
      }
    }
  }

  private struct QRCodeViewModel {
    let name: String
    let address: String
  }

  @ViewBuilder private func qrCodeView(_ viewModel: QRCodeViewModel) -> some View {
    VStack(spacing: 12) {
      RoundedRectangle(cornerRadius: 20, style: .continuous)
        .stroke(Color(.secondaryButtonTint).opacity(0.5), lineWidth: 1)
        .frame(width: 184, height: 184)
        .overlay(
          Group {
            if let image = viewModel.address.qrCodeImage?.cgImage {
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
      Text(viewModel.name)
        .font(.callout.weight(.semibold))
        .multilineTextAlignment(.center)
      Text(viewModel.address)
        .font(.subheadline)
        .foregroundColor(Color(.secondaryBraveLabel))
        .multilineTextAlignment(.center)
      HStack {
        Button {
          UIPasteboard.general.string = viewModel.address
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
          addressToShare = viewModel.address
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
      supportedNetworks
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
        case .prefilledAccount(let account, let bitcoinAccounts):
          depositHeader(
            coin: account.coin,
            networks: supportedNetworks
          )
          qrCodeView(buildQRCodeViewModel(account: account, bitcoinAccounts: bitcoinAccounts))
          if account.coin == .eth {
            ethDisclosureView
          }
        case .prefilledToken(let token, _, let bitcoinAccounts):
          Button {
            isPresentingAccountPicker = true
          } label: {
            accountPicker
          }
          depositHeader(
            coin: token.coin,
            networks: supportedNetworks
          )
          if let selectedAccount {
            qrCodeView(
              buildQRCodeViewModel(
                account: selectedAccount,
                bitcoinAccounts: bitcoinAccounts
              )
            )
          }
          if token.coin == .eth {
            ethDisclosureView
          }
        }
      }
      .padding(.vertical, 32)
      .padding(.horizontal, 16)
      .sheet(isPresented: $isPresentingAccountPicker) {
        if let selectedAccount, case .prefilledToken(_, let availableAccounts, _) = type {
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

  private func buildQRCodeViewModel(
    account: BraveWallet.AccountInfo,
    bitcoinAccounts: [String: BraveWallet.BitcoinAccountInfo]
  ) -> QRCodeViewModel {
    if let bitcoinAccount = bitcoinAccounts[account.accountId.uniqueKey] {
      return .init(name: account.name, address: bitcoinAccount.nextReceiveAddress.addressString)
    } else {
      return .init(name: account.name, address: account.address)
    }
  }
}

extension Array where Element == BraveWallet.NetworkInfo {
  fileprivate func supportedNetworks(keyringId: BraveWallet.KeyringId) -> [BraveWallet.NetworkInfo]
  {
    return self.filter {
      $0.supportedKeyrings.contains(keyringId.rawValue as NSNumber)
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
