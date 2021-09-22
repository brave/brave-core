/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import BraveCore
import Combine
import SwiftUI
import BraveUI

struct AccountsView: View {
  @ObservedObject var keyringStore: KeyringStore
  @State private var navigationController: UINavigationController?
  
  private var primaryAccounts: [BraveWallet.AccountInfo] {
    keyringStore.keyring.accountInfos.filter { !$0.isImported }
  }
  
  private var secondaryAccounts: [BraveWallet.AccountInfo] {
    keyringStore.keyring.accountInfos.filter(\.isImported)
  }
  
  @ViewBuilder
  private func accountView(for account: BraveWallet.AccountInfo) -> some View {
    // Using `NavigationLink` in iOS 14.0 here ends up with a bug where the cell doesn't deselect because
    // the navigation is a UINavigationController and this view is inside of a UIPageViewController
    let view = AccountView(address: account.address, name: account.name)
    let destination = AccountActivityView(keyringStore: keyringStore, account: account)
    if #available(iOS 15.0, *) {
      ZStack {
        view
        NavigationLink(destination: destination) {
          EmptyView()
        }
        .opacity(0) // Design doesnt have a disclosure icon
      }
    } else {
      Button(action: {
        navigationController?.pushViewController(
          UIHostingController(rootView: destination),
          animated: true
        )
      }) {
        view
      }
    }
  }
  
  var body: some View {
    List {
      Section(
        header: AccountsHeaderView(keyringStore: keyringStore)
          .resetListHeaderStyle()
      ) {
      }
      Section(
        header: WalletListHeaderView(
          title: Text("Primary Crypto Accounts") // NSLocalizedString
        )
      ) {
        ForEach(primaryAccounts) { account in
          accountView(for: account)
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      Section(
        header: WalletListHeaderView(
          title: Text("Secondary Accounts"), // NSLocalizedString
          subtitle: Text("Import your external wallet account with a separate seed phrase.") // NSLocalizedString
        )
      ) {
        let secondaryAccounts = secondaryAccounts
        if secondaryAccounts.isEmpty {
          Text("No secondary accounts") // NSLocalizedString
            .foregroundColor(Color(.secondaryBraveLabel))
            .multilineTextAlignment(.center)
            .frame(maxWidth: .infinity)
            .font(.footnote.weight(.medium))
        } else {
          ForEach(secondaryAccounts) { account in
            accountView(for: account)
          }
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .listStyle(InsetGroupedListStyle())
    .navigationTitle("Accounts") // NSLocalizedString
    .osAvailabilityModifiers { content in
      if #available(iOS 15.0, *) {
        content
      } else {
        content
          .introspectNavigationController { nc in
            navigationController = nc
          }
      }
    }
  }
}

#if DEBUG
struct AccountsViewController_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      AccountsView(keyringStore: .previewStoreWithWalletCreated)
    }
    .previewLayout(.sizeThatFits)
    .previewColorSchemes()
  }
}
#endif
