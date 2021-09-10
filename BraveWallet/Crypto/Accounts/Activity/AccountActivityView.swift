/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveCore
import SwiftUI
import BraveUI

@available(iOS 14.0, *)
struct AccountActivityView: View {
  @ObservedObject var keyringStore: KeyringStore
  var account: BraveWallet.AccountInfo
  
  @State private var detailsPresentation: DetailsPresentation?
  @Environment(\.presentationMode) @Binding private var presentationMode
  
  private struct DetailsPresentation: Identifiable {
    var inEditMode: Bool
    var id: String {
      "\(inEditMode)"
    }
  }
  
  private var accountInfo: BraveWallet.AccountInfo {
    guard let info = keyringStore.keyring.accountInfos.first(where: { $0.address == account.address }) else {
      // The account has been removed... User should technically never see this state because
      // `AccountsViewController` pops this view off the stack when the account is removed
      return account
    }
    return info
  }
  
  private func emptyTextView(_ message: String) -> some View {
    Text(message)
      .font(.footnote.weight(.medium))
      .frame(maxWidth: .infinity)
      .multilineTextAlignment(.center)
      .foregroundColor(Color(.secondaryBraveLabel))
  }
  
  var body: some View {
    List {
      Section {
        AccountActivityHeaderView(account: accountInfo) { editMode in
          // Needed to use an identifiable object here instead of two @State Bool's due to a SwiftUI bug
          detailsPresentation = .init(inEditMode: editMode)
        }
        .frame(maxWidth: .infinity)
        .listRowInsets(.zero)
        .listRowBackground(Color(.braveGroupedBackground))
      }
      Section(
        header: WalletListHeaderView(title: Text("Assets")) // NSLocalizedString
      ) {
        emptyTextView("No Assets") // NSLocalizedString
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      Section(
        header: WalletListHeaderView(title: Text("Transactions")) // NSLocalizedString
      ) {
        emptyTextView("No Transactions") // NSLocalizedString
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .listStyle(InsetGroupedListStyle())
    .sheet(item: $detailsPresentation) {
      AccountDetailsView(
        keyringStore: keyringStore,
        account: accountInfo,
        editMode: $0.inEditMode
      )
    }
    .onReceive(keyringStore.$keyring) { keyring in
      if !keyring.accountInfos.contains(where: { $0.address == accountInfo.address }) {
        // Account was deleted
        detailsPresentation = nil
        presentationMode.dismiss()
      }
    }
  }
}

@available(iOS 14.0, *)
private struct AccountActivityHeaderView: View {
  var account: BraveWallet.AccountInfo
  var action: (_ tappedEdit: Bool) -> Void
  
  var body: some View {
    VStack {
      Blockie(address: account.address)
        .frame(width: 64, height: 64)
      VStack(spacing: 4) {
        Text(account.name)
          .fontWeight(.semibold)
        Text(account.address.truncatedAddress)
          .font(.footnote)
      }
      .padding(.bottom, 12)
      HStack {
        Button(action: { action(false) }) {
          HStack {
            Image("brave.qr-code")
              .font(.body)
            Text("Details") // NSLocalizedString
              .font(.footnote.weight(.bold))
          }
        }
        Button(action: { action(true) }) {
          HStack {
            Image("brave.edit")
              .font(.body)
            Text("Rename") // NSLocalizedString
              .font(.footnote.weight(.bold))
          }
        }
      }
      .buttonStyle(BraveOutlineButtonStyle(size: .normal))
    }
    .padding()
  }
}

#if DEBUG
@available(iOS 14.0, *)
struct AccountActivityView_Previews: PreviewProvider {
  static var previews: some View {
    AccountActivityView(
      keyringStore: .previewStoreWithWalletCreated,
      account: KeyringStore.previewStoreWithWalletCreated.keyring.accountInfos.first!
    )
    .previewColorSchemes()
  }
}
#endif
