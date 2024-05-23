// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveUI
import Data
import Strings
import SwiftUI

struct EditSiteConnectionView: View {
  @ObservedObject var keyringStore: KeyringStore
  var origin: URLOrigin
  var coin: BraveWallet.CoinType
  var onDismiss: (_ permittedAccounts: [String]) -> Void

  @Environment(\.sizeCategory) private var sizeCategory

  @ScaledMetric private var faviconSize = 48
  private let maxFaviconSize: CGFloat = 96

  @State private var permittedAccounts: [String] = []

  enum EditAction {
    case connect(_ coin: BraveWallet.CoinType)
    case disconnect(_ coin: BraveWallet.CoinType)
    case `switch`

    var title: String {
      switch self {
      case .connect(let coin):
        return coin == .eth
          ? Strings.Wallet.editSiteConnectionAccountActionConnect
          : Strings.Wallet.editSiteConnectionAccountActionTrust
      case .disconnect(let coin):
        return coin == .eth
          ? Strings.Wallet.editSiteConnectionAccountActionDisconnect
          : Strings.Wallet.editSiteConnectionAccountActionRevoke
      case .switch:
        return Strings.Wallet.editSiteConnectionAccountActionSwitch
      }
    }
  }

  private func editAction(for account: BraveWallet.AccountInfo) -> EditAction {
    if permittedAccounts.contains(account.address) {
      if keyringStore.selectedAccount.id == account.id {
        // Disconnect - Connected and selected account
        return .disconnect(coin)
      } else {
        if coin == .sol {
          return .disconnect(coin)
        }
        // Switch - Connected but not selected account (only for ethereum)
        return .`switch`
      }
    } else {
      // Connect - Not connected
      return .connect(coin)
    }
  }

  private var connectedAddresses: String {
    let account = Strings.Wallet.editSiteConnectionAccountSingular
    let accounts = Strings.Wallet.editSiteConnectionAccountPlural
    return String.localizedStringWithFormat(
      Strings.Wallet.editSiteConnectionConnectedAccount,
      permittedAccounts.count,
      permittedAccounts.count == 1 ? account : accounts
    )
  }

  @ViewBuilder private func editButton(
    action: EditAction,
    account: BraveWallet.AccountInfo
  ) -> some View {
    Button {
      switch action {
      case .connect:
        if let url = origin.url {
          Domain.setWalletPermissions(
            forUrl: url,
            coin: coin,
            accounts: [account.address],
            grant: true
          )
        }
        permittedAccounts.append(account.address)

        if coin == .eth {  // only for eth dapp connection can be triggered on the wallet side
          keyringStore.selectedAccount = account
        }
      case .disconnect:
        if let url = origin.url {
          Domain.setWalletPermissions(
            forUrl: url,
            coin: coin,
            accounts: [account.address],
            grant: false
          )
        }
        permittedAccounts.removeAll(where: { $0 == account.address })

        if coin == .eth {  // only for eth dapp connection can be triggered on the wallet side
          if let firstAllowedAdd = permittedAccounts.first,
            let firstAllowedAccount = accountInfos.first(where: { $0.id == firstAllowedAdd })
          {
            keyringStore.selectedAccount = firstAllowedAccount
          }
        }
      case .switch:
        // So far switch should only be an action for eth accounts
        if coin == .eth {  // only for eth dapp connection can be triggered on the wallet side
          keyringStore.selectedAccount = account
        }
      }
    } label: {
      Text(action.title)
        .foregroundColor(Color(.braveBlurpleTint))
        .font(.footnote.weight(.semibold))
    }
  }

  @ViewBuilder private func connectionInfo(urlOrigin: URLOrigin) -> some View {
    urlOrigin.url.map { url in
      FaviconReader(url: url) { image in
        if let image = image {
          Image(uiImage: image)
            .resizable()
            .scaledToFit()
            .background(Color(.braveDisabled))
            .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
        } else {
          ProgressView()
        }
      }
      .frame(width: min(faviconSize, maxFaviconSize), height: min(faviconSize, maxFaviconSize))
    }
    VStack(alignment: sizeCategory.isAccessibilityCategory ? .center : .leading, spacing: 2) {
      Text(urlOrigin: urlOrigin)
        .font(.subheadline)
        .foregroundColor(Color(.bravePrimary))
      Text(connectedAddresses)
        .font(.footnote)
        .multilineTextAlignment(.center)
        .foregroundColor(Color(.braveLabel))
    }
  }

  private var accountInfos: [BraveWallet.AccountInfo] {
    keyringStore.allAccounts.filter { $0.coin == coin }
  }

  var body: some View {
    NavigationView {
      Form {
        Section {
          ForEach(accountInfos) { account in
            let action = editAction(for: account)
            if sizeCategory.isAccessibilityCategory {
              VStack {
                AddressView(address: account.address) {
                  AccountView(account: account)
                }
                Spacer()
                editButton(action: action, account: account)
              }
            } else {
              HStack {
                AddressView(address: account.address) {
                  AccountView(account: account)
                }
                Spacer()
                editButton(action: action, account: account)
              }
            }
          }
          .listRowBackground(Color(.secondaryBraveGroupedBackground))
        } header: {
          Group {
            if sizeCategory.isAccessibilityCategory {
              VStack(spacing: 12) {
                connectionInfo(urlOrigin: origin)
              }
            } else {
              HStack(spacing: 12) {
                connectionInfo(urlOrigin: origin)
              }
            }
          }
          .frame(maxWidth: .infinity, alignment: .leading)
          .resetListHeaderStyle()
          .padding(.vertical)
        }
      }
      .listBackgroundColor(Color(UIColor.braveGroupedBackground))
      .navigationTitle(Strings.Wallet.editSiteConnectionScreenTitle)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItemGroup(placement: .confirmationAction) {
          Button {
            onDismiss(permittedAccounts)
          } label: {
            Text(Strings.done)
              .foregroundColor(Color(.braveBlurpleTint))
          }
        }
      }
      .onAppear {
        if let url = origin.url, let accounts = Domain.walletPermissions(forUrl: url, coin: coin) {
          permittedAccounts = accounts
        }
      }
    }
    .navigationViewStyle(.stack)
  }
}

#if DEBUG
struct EditSiteConnectionView_Previews: PreviewProvider {
  static var previews: some View {
    EditSiteConnectionView(
      keyringStore: .previewStoreWithWalletCreated,
      origin: .init(url: URL(string: "https://app.uniswap.org")!),
      coin: .eth,
      onDismiss: { _ in }
    )
  }
}
#endif
