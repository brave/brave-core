// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveCore
import Strings
import BraveShared
import BraveUI
import Data

struct EditSiteConnectionView: View {
  @ObservedObject var keyringStore: KeyringStore
  var origin: URLOrigin
  var onDismiss: (_ permittedAccounts: [String]) -> Void
  
  @Environment(\.sizeCategory) private var sizeCategory
  
  @ScaledMetric private var faviconSize = 48
  private let maxFaviconSize: CGFloat = 96
  
  @State private var permittedAccounts: [String] = []
  
  enum EditAction {
    case connect
    case disconnect
    case `switch`
    
    var title: String {
      switch self {
      case .connect:
        return Strings.Wallet.editSiteConnectionAccountActionConnect
      case .disconnect:
        return Strings.Wallet.editSiteConnectionAccountActionDisconnect
      case .switch:
        return Strings.Wallet.editSiteConnectionAccountActionSwitch
      }
    }
  }
  
  private func editAction(for account: BraveWallet.AccountInfo) -> EditAction {
    if permittedAccounts.contains(account.address) {
      if keyringStore.selectedAccount.id == account.id {
        // Disconnect - Connected and selected account
        return .disconnect
      } else {
        // Switch - Connected but not selected account
        return .`switch`
      }
    } else {
      // Connect - Not connected
      return .connect
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
  
  @ViewBuilder private func editButton(action: EditAction, account: BraveWallet.AccountInfo) -> some View {
    Button {
      switch action {
      case .connect:
        if let url = origin.url {
          Domain.setEthereumPermissions(forUrl: url, account: account.address, grant: true)
        }
        permittedAccounts.append(account.address)
        keyringStore.selectedAccount = account
      case .disconnect:
        if let url = origin.url {
          Domain.setEthereumPermissions(forUrl: url, account: account.address, grant: false)
        }
        permittedAccounts.removeAll(where: { $0 == account.address })
        
        if let firstAllowedAdd = permittedAccounts.first, let firstAllowedAccount = keyringStore.keyring.accountInfos.first(where: { $0.id == firstAllowedAdd }) {
          keyringStore.selectedAccount = firstAllowedAccount
        }
      case .switch:
        keyringStore.selectedAccount = account
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
            .frame(width: min(faviconSize, maxFaviconSize), height: min(faviconSize, maxFaviconSize))
            .background(Color(.braveDisabled))
            .clipShape(RoundedRectangle(cornerRadius: 10, style: .continuous))
        } else {
          ProgressView()
        }
      }
    }
    VStack(alignment: sizeCategory.isAccessibilityCategory ? .center : .leading, spacing: 2) {
      Text(urlOrigin: urlOrigin)
        .font(.subheadline)
        .multilineTextAlignment(.center)
        .foregroundColor(Color(.bravePrimary))
      Text(connectedAddresses)
        .font(.footnote)
        .multilineTextAlignment(.center)
        .foregroundColor(Color(.braveLabel))
    }
  }
  
  var body: some View {
    NavigationView {
      Form {
        Section {
          ForEach(keyringStore.keyring.accountInfos, id: \.self) { account in
            let action = editAction(for: account)
            if sizeCategory.isAccessibilityCategory {
              VStack {
                AddressView(address: account.address) {
                  AccountView(address: account.address, name: account.name)
                }
                Spacer()
                editButton(action: action, account: account)
              }
            } else {
              HStack {
                AddressView(address: account.address) {
                  AccountView(address: account.address, name: account.name)
                }
                Spacer()
                editButton(action: action, account: account)
              }
            }
          }
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
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      }
      .navigationTitle(Strings.Wallet.editSiteConnectionScreenTitle)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItemGroup(placement: .confirmationAction) {
          Button {
            onDismiss(permittedAccounts)
          } label: {
            Text(Strings.done)
              .foregroundColor(Color(.braveOrange))
          }
        }
      }
      .onAppear {
        if let url = origin.url, let accounts = Domain.ethereumPermissions(forUrl: url) {
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
      keyringStore: {
        let store = KeyringStore.previewStoreWithWalletCreated
        store.addPrimaryAccount("Account 2", completion: nil)
        store.addPrimaryAccount("Account 3", completion: nil)
        return store
      }(),
      origin: .init(url: URL(string: "https://app.uniswap.org")!),
      onDismiss: { _ in }
    )
  }
}
#endif
