// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI

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
          Text("Reset") // NSLocalizedString
            .foregroundColor(.red)
        }
        // iOS 15: .role(.destructive)
      }
    }
    .listStyle(InsetGroupedListStyle())
    .navigationTitle("Wallet") // NSLocalizedString
    .navigationBarTitleDisplayMode(.inline)
    // TODO: Needs real copy for whole reset prompt
    .alert(isPresented: $isShowingResetAlert) {
      Alert(
        title: Text("Reset Wallet"), // NSLocalizedString
        message: Text("Are you sure you want to reset your wallet? If you have not backed up your recovery prhase you will not be able to restore it at a later time."), // NSLocalizedString
        primaryButton: .destructive(Text("Reset"), action: { // NSLocalizedString
          keyringStore.reset()
        }),
        secondaryButton: .cancel(Text("No")) // NSLocalizedString
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
