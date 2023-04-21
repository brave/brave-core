/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI
import Strings
import DesignSystem
import BraveCore

struct AccountsHeaderView: View {
  @ObservedObject var keyringStore: KeyringStore
  var settingsStore: SettingsStore
  var networkStore: NetworkStore

  @State private var isPresentingBackup: Bool = false
  @State private var isPresentingAddAccount: Bool = false

  var body: some View {
    HStack {
      Button(action: { isPresentingBackup = true }) {
        HStack {
          Image(braveSystemName: "leo.safe")
            .foregroundColor(Color(.braveLabel))
          Text(Strings.Wallet.accountBackup)
            .font(.subheadline.weight(.medium))
            .foregroundColor(Color(.braveBlurpleTint))
        }
      }
      .background(
        Color.clear
          .sheet(isPresented: $isPresentingBackup) {
            NavigationView {
              BackupWalletView(
                password: nil,
                keyringStore: keyringStore
              )
            }
            .navigationViewStyle(StackNavigationViewStyle())
            .environment(\.modalPresentationMode, $isPresentingBackup)
            .accentColor(Color(.braveBlurpleTint))
          }
      )
      Spacer()
      HStack(spacing: 16) {
        Button(action: {
          isPresentingAddAccount = true
        }) {
          Label(Strings.Wallet.addAccountTitle, systemImage: "plus")
            .labelStyle(.iconOnly)
        }
        .background(
          Color.clear
            .sheet(isPresented: $isPresentingAddAccount) {
              NavigationView {
                AddAccountView(keyringStore: keyringStore)
              }
              .navigationViewStyle(StackNavigationViewStyle())
            }
        )
        NavigationLink(
          destination: Web3SettingsView(
            settingsStore: settingsStore,
            networkStore: networkStore,
            keyringStore: keyringStore)
        ) {
          Label(Strings.Wallet.settings, braveSystemImage: "leo.settings")
            .labelStyle(.iconOnly)
        }
      }
      .foregroundColor(Color(.braveLabel))
    }
    .padding(.top)
  }
}

#if DEBUG
struct AccountsHeaderView_Previews: PreviewProvider {
  static var previews: some View {
    AccountsHeaderView(
      keyringStore: .previewStore,
      settingsStore: .previewStore,
      networkStore: .previewStore
    )
    .previewLayout(.sizeThatFits)
    .previewColorSchemes()
  }
}
#endif
