// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import struct Shared.Strings

public struct WalletSettingsView: View {
  @ObservedObject var keyringStore: KeyringStore
  
  @State private var isShowingResetAlert = false
  
  public init(keyringStore: KeyringStore) {
    self.keyringStore = keyringStore
  }
  
  public var body: some View {
    List {
      Section {
        Button(action: { isShowingResetAlert = true }) {
          Text(Strings.Wallet.settingsResetButtonTitle)
            .foregroundColor(.red)
        }
        // iOS 15: .role(.destructive)
      }
    }
    .listStyle(InsetGroupedListStyle())
    .navigationTitle("Brave Wallet")
    .navigationBarTitleDisplayMode(.inline)
    // TODO: Needs real copy for whole reset prompt
    .alert(isPresented: $isShowingResetAlert) {
      Alert(
        title: Text(Strings.Wallet.settingsResetWalletAlertTitle),
        message: Text(Strings.Wallet.settingsResetWalletAlertMessage),
        primaryButton: .destructive(Text(Strings.Wallet.settingsResetWalletAlertButtonTitle), action: {
          keyringStore.reset()
        }),
        secondaryButton: .cancel(Text(Strings.no))
      )
    }
  }
}

#if DEBUG
struct WalletSettingsView_Previews: PreviewProvider {
  static var previews: some View {
    NavigationView {
      WalletSettingsView(keyringStore: .previewStore)
    }
  }
}
#endif
