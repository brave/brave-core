// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import Strings
import SwiftUI

struct BiometricsPasscodeEntryView: View {

  @ObservedObject var keyringStore: KeyringStore
  var settingsStore: SettingsStore

  var body: some View {
    PasswordEntryView(
      keyringStore: keyringStore,
      title: Strings.Wallet.enterPasswordForBiometricsNavTitle,
      message: Strings.Wallet.enterPasswordForBiometricsTitle,
      action: { password, completion in
        keyringStore.validate(password: password) { [weak settingsStore] isValid in
          defer {
            settingsStore?.updateBiometricsToggle()
          }
          if isValid {
            // store password in keychain
            if case let status = keyringStore.storePasswordInKeychain(password),
              status != errSecSuccess
            {
              // Failed to store password in keychain
              completion(.failedToEnableBiometrics)
            } else {
              // Successfully stored password in keychain
              completion(nil)
            }
          } else {  // incorrect/invalid password
            // Conflict with the keyboard submit/dismissal that causes a bug
            // with SwiftUI animating the screen away...
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.05) {
              completion(.incorrectPassword)
            }
          }
        }
      }
    )
  }
}

#if DEBUG
struct BiometricsPasscodeEntryView_Previews: PreviewProvider {
  static var previews: some View {
    BiometricsPasscodeEntryView(
      keyringStore: .previewStore,
      settingsStore: .previewStore
    )
  }
}
#endif
