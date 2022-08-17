// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import Strings
import BraveShared
import DesignSystem
import BraveCore

/// A view to display to a user to allow them to setup a connection to a dApp for the first time.
public struct NewSiteConnectionView: View {
  @ObservedObject var keyringStore: KeyringStore
  var origin: URLOrigin
  var onConnect: (_ addresses: [String]) -> Void
  
  @available(iOS, introduced: 14.0, deprecated: 15.0, message: "Use PresentationMode on iOS 15")
  var onDismiss: () -> Void
  
  public init(
    origin: URLOrigin,
    keyringStore: KeyringStore,
    onConnect: @escaping (_ addresses: [String]) -> Void,
    onDismiss: @escaping () -> Void
  ) {
    self.origin = origin
    self.keyringStore = keyringStore
    self.onConnect = onConnect
    self.onDismiss = onDismiss
  }

  @ScaledMetric private var faviconSize = 48
  private let maxFaviconSize: CGFloat = 96
  @State private var selectedAccounts: Set<BraveWallet.AccountInfo.ID> = []
  @State private var isConfirmationViewVisible: Bool = false
  
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
        }
        .listRowBackground(Color(.braveGroupedBackground))
        Section {
          ForEach(keyringStore.defaultKeyring.accountInfos) { account in
            Button {
              if selectedAccounts.contains(account.id) {
                selectedAccounts.remove(account.id)
              } else {
                selectedAccounts.insert(account.id)
              }
            } label: {
              HStack {
                AddressView(address: account.address) {
                  AccountView(address: account.address, name: account.name)
                }
                Spacer()
                Group {
                  if selectedAccounts.contains(account.id) {
                    Image(braveSystemName: "brave.checkmark.circle.fill")
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
        } header: {
          WalletListHeaderView(title: Text(Strings.Wallet.accountsPageTitle))
        } footer: {
          cautionFooterView
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
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
            NavigationLink(isActive: $isConfirmationViewVisible, destination: {
              confirmationView
            }, label: {
              EmptyView()
            })
            .hidden()
          )
        }
        .listRowBackground(Color(.braveGroupedBackground))
      }
      .listStyle(InsetGroupedListStyle())
      .navigationBarTitleDisplayMode(.inline)
      .navigationTitle(Strings.Wallet.newSiteConnectScreenTitle)
      .toolbar {
        ToolbarItemGroup(placement: .cancellationAction) {
          Button {
            onDismiss()
          } label: {
            Text(Strings.cancelButtonTitle)
              .foregroundColor(Color(.braveOrange))
          }
        }
      }
    }
    .navigationViewStyle(.stack)
    .onAppear {
      if keyringStore.selectedAccount.coin == .eth {
        selectedAccounts.insert(keyringStore.selectedAccount.id)
      } else { // fetch selected account for `.eth` coin type
        Task { @MainActor in
          if let selectedEthAccount = await keyringStore.selectedAccount(for: .eth) {
            selectedAccounts.insert(selectedEthAccount.id)
          }
        }
      }
    }
  }
  
  private var accountsAddressesToConfirm: String {
    keyringStore.defaultKeyring.accountInfos
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
      }
      .listRowBackground(Color(.braveGroupedBackground))
      Section {
        HStack(spacing: 12) {
          Image(braveSystemName: "brave.checkmark.circle.fill")
            .imageScale(.large)
          Text(Strings.Wallet.newSiteConnectConfirmationMessage)
            .multilineTextAlignment(.leading)
        }
        .foregroundColor(Color(.braveLabel))
        .frame(maxWidth: .infinity)
        .accessibilityElement(children: .combine)
      } footer: {
        cautionFooterView
      }
      .listRowBackground(Color(.braveGroupedBackground))
      Section {
        Button {
          let accounts = keyringStore.defaultKeyring.accountInfos
            .filter { selectedAccounts.contains($0.id) }
            .map(\.address)
          onConnect(accounts)
        } label: {
          Text(Strings.Wallet.confirm)
        }
        .buttonStyle(BraveFilledButtonStyle(size: .large))
        .frame(maxWidth: .infinity)
      }
      .listRowBackground(Color(.braveGroupedBackground))
    }
    .listStyle(InsetGroupedListStyle())
    .navigationTitle(Strings.Wallet.newSiteConnectScreenTitle)
    .navigationBarTitleDisplayMode(.inline)
  }
}

#if DEBUG
struct NewSiteConnectionView_Previews: PreviewProvider {
  static var previews: some View {
    NewSiteConnectionView(
      origin: .init(url: URL(string: "https://app.uniswap.org")!),
      keyringStore: {
        let store = KeyringStore.previewStoreWithWalletCreated
        store.addPrimaryAccount("Account 2", coin: .eth, completion: nil)
        store.addPrimaryAccount("Account 3", coin: .eth, completion: nil)
        return store
      }(),
      onConnect: { _ in },
      onDismiss: { }
    )
  }
}
#endif
