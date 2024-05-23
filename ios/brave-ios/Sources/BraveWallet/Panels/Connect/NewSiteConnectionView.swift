// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveUI
import DesignSystem
import Strings
import SwiftUI

/// A view to display to a user to allow them to setup a connection to a dApp for the first time.
public struct NewSiteConnectionView: View {
  @ObservedObject var keyringStore: KeyringStore
  var origin: URLOrigin
  var accounts: [String]
  var coin: BraveWallet.CoinType
  var onConnect: (_ addresses: [String]) -> Void
  var onDismiss: () -> Void

  public init(
    origin: URLOrigin,
    accounts: [String],
    coin: BraveWallet.CoinType,
    keyringStore: KeyringStore,
    onConnect: @escaping (_ addresses: [String]) -> Void,
    onDismiss: @escaping () -> Void
  ) {
    self.origin = origin
    self.accounts = accounts
    self.coin = coin
    self.keyringStore = keyringStore
    self.onConnect = onConnect
    self.onDismiss = onDismiss
  }

  @ScaledMetric private var faviconSize = 48
  private let maxFaviconSize: CGFloat = 96
  @State private var selectedAccounts: Set<BraveWallet.AccountInfo.ID> = []
  @State private var isConfirmationViewVisible: Bool = false

  private var accountInfos: [BraveWallet.AccountInfo] {
    let allAccounts = keyringStore.allAccounts.filter { $0.coin == coin }
    return allAccounts.filter { self.accounts.contains($0.address) }
  }

  @ViewBuilder private func originAndFavicon(urlOrigin: URLOrigin) -> some View {
    urlOrigin.url.map { url in
      FaviconReader(url: url) { image in
        if let image = image {
          Image(uiImage: image)
            .resizable()
            .scaledToFit()
            .background(Color(.braveDisabled))
            .clipShape(RoundedRectangle(cornerRadius: 4, style: .continuous))
        } else {
          ProgressView()
        }
      }
      .frame(width: min(faviconSize, maxFaviconSize), height: min(faviconSize, maxFaviconSize))
    }
    Text(urlOrigin: urlOrigin)
      .font(.subheadline)
      .foregroundColor(Color(.braveLabel))
      .multilineTextAlignment(.center)
  }

  private var headerView: some View {
    VStack(spacing: 8) {
      originAndFavicon(urlOrigin: origin)
        .accessibilityElement(children: .combine)
      Text(Strings.Wallet.newSiteConnectMessage)
        .font(.headline)
        .foregroundColor(Color(.bravePrimary))
        .multilineTextAlignment(.center)
    }
    .frame(maxWidth: .infinity)
  }

  private var cautionFooterView: some View {
    Text(Strings.Wallet.newSiteConnectFooter)
      .frame(maxWidth: .infinity)
      .padding(.top)
      .foregroundColor(Color(.braveLabel))
      .multilineTextAlignment(.center)
  }

  public var body: some View {
    NavigationView {
      List {
        Section {
          headerView
            .listRowBackground(Color(.braveGroupedBackground))
        }
        Section {
          ForEach(accountInfos) { account in
            Button {
              switch coin {
              case .eth:
                if selectedAccounts.contains(account.id) {
                  selectedAccounts.remove(account.id)
                } else {
                  selectedAccounts.insert(account.id)
                }
              case .sol:
                // only allow selecting one Solana account at a time
                selectedAccounts = .init(arrayLiteral: account.id)
              default:
                break  // not supported
              }
            } label: {
              HStack {
                AddressView(address: account.address) {
                  AccountView(
                    address: account.address,
                    seed: account.blockieSeed,
                    name: account.name
                  )
                }
                Spacer()
                Group {
                  if selectedAccounts.contains(account.id) {
                    Image(braveSystemName: "leo.check.circle-filled")
                      .foregroundColor(Color(.braveSuccessLabel))
                  } else {
                    Image(systemName: "circle")
                      .foregroundColor(Color(.secondaryButtonTint))
                  }
                }
                .imageScale(.large)
                .transition(.opacity)
              }
            }
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        } header: {
          WalletListHeaderView(title: Text(Strings.Wallet.accountsPageTitle))
        } footer: {
          cautionFooterView
        }
        Section {
          Button {
            isConfirmationViewVisible = true
          } label: {
            Text(Strings.Wallet.next)
          }
          .buttonStyle(BraveFilledButtonStyle(size: .large))
          .disabled(selectedAccounts.isEmpty)
          .animation(.default, value: selectedAccounts.isEmpty)
          .frame(maxWidth: .infinity)
          .background(
            NavigationLink(
              isActive: $isConfirmationViewVisible,
              destination: {
                confirmationView
              },
              label: {
                EmptyView()
              }
            )
            .hidden()
          )
          .listRowBackground(Color(.braveGroupedBackground))
        }
      }
      .listStyle(InsetGroupedListStyle())
      .listBackgroundColor(Color(UIColor.braveGroupedBackground))
      .navigationBarTitleDisplayMode(.inline)
      .navigationTitle(Strings.Wallet.newSiteConnectScreenTitle)
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
    }
    .navigationViewStyle(.stack)
    .onAppear {
      if accounts.contains(keyringStore.selectedAccount.address),
        keyringStore.selectedAccount.coin == coin
      {
        // currently selected account exists in permissions request, select it
        selectedAccounts.insert(keyringStore.selectedAccount.id)
      } else {  // Need to fetch selected account for coin
        Task { @MainActor in
          if let selectedAccount = await keyringStore.selectedDappAccount(for: coin) {
            if accounts.contains(selectedAccount.address) {
              // currently selected account exists in permissions request, select it
              selectedAccounts.insert(selectedAccount.id)
            } else if let firstAccount = accounts.first {
              // else select the first account by default
              selectedAccounts.insert(firstAccount)
            }
          }
        }
      }
    }
  }

  private var accountsAddressesToConfirm: String {
    accountInfos
      .filter { selectedAccounts.contains($0.id) }
      .map(\.address.truncatedAddress)
      .joined(separator: ", ")
  }

  @ViewBuilder private var confirmationView: some View {
    List {
      Section {
        VStack(spacing: 8) {
          headerView
          Text(accountsAddressesToConfirm)
            .multilineTextAlignment(.center)
            .font(.footnote)
            .foregroundColor(Color(.braveLabel))
        }
        .listRowBackground(Color(.braveGroupedBackground))
      }
      Section {
        HStack(spacing: 12) {
          Image(braveSystemName: "leo.check.circle-filled")
            .imageScale(.large)
          Text(Strings.Wallet.newSiteConnectConfirmationMessage)
            .multilineTextAlignment(.leading)
        }
        .foregroundColor(Color(.braveLabel))
        .frame(maxWidth: .infinity)
        .accessibilityElement(children: .combine)
        .listRowBackground(Color(.braveGroupedBackground))
      } footer: {
        cautionFooterView
      }
      Section {
        Button {
          let accounts =
            accountInfos
            .filter { selectedAccounts.contains($0.id) }
            .map(\.address)
          onConnect(accounts)
        } label: {
          Text(Strings.Wallet.confirm)
        }
        .buttonStyle(BraveFilledButtonStyle(size: .large))
        .frame(maxWidth: .infinity)
        .listRowBackground(Color(.braveGroupedBackground))
      }
    }
    .listStyle(InsetGroupedListStyle())
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .navigationTitle(Strings.Wallet.newSiteConnectScreenTitle)
    .navigationBarTitleDisplayMode(.inline)
  }
}

#if DEBUG
struct NewSiteConnectionView_Previews: PreviewProvider {
  static var previews: some View {
    NewSiteConnectionView(
      origin: .init(url: URL(string: "https://app.uniswap.org")!),
      accounts: [BraveWallet.AccountInfo.previewAccount.address],
      coin: .eth,
      keyringStore: .previewStoreWithWalletCreated,
      onConnect: { _ in },
      onDismiss: {}
    )
  }
}
#endif
