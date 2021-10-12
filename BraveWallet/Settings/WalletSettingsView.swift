// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import struct Shared.Strings
import BraveUI

public struct WalletSettingsView: View {
  @ObservedObject var keyringStore: KeyringStore
  
  @State private var isShowingResetAlert = false
  
  public init(keyringStore: KeyringStore) {
    self.keyringStore = keyringStore
  }
  
  private var autoLockIntervals: [AutoLockInterval] {
    var all = AutoLockInterval.allOptions
    if !all.contains(keyringStore.autoLockInterval) {
      // Ensure that the users selected interval always appears as an option even if
      // we remove it from `allOptions`
      all.append(keyringStore.autoLockInterval)
    }
    return all.sorted(by: { $0.value < $1.value })
  }
  
  public var body: some View {
    Form {
      Section(
        footer: Text(Strings.Wallet.autoLockFooter)
          .foregroundColor(Color(.secondaryBraveLabel))
      ) {
        Picker(selection: $keyringStore.autoLockInterval) {
          ForEach(autoLockIntervals) { interval in
            Text(interval.label)
              .tag(interval)
          }
        } label: {
          Text(Strings.Wallet.autoLockTitle)
            .padding(.vertical, 4)
        }
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      Section {
        Button(action: { isShowingResetAlert = true }) {
          Text(Strings.Wallet.settingsResetButtonTitle)
            .foregroundColor(.red)
        }
        // iOS 15: .role(.destructive)
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
    }
    .listStyle(InsetGroupedListStyle())
    .navigationTitle("Brave Wallet")
    .navigationBarTitleDisplayMode(.inline)
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
