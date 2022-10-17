// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import DesignSystem
import Strings
import SwiftUI

struct BiometricsPasscodeEntryView: View {
  
  @ObservedObject var keyringStore: KeyringStore
  
  var body: some View {
    PasswordEntryView(
      keyringStore: keyringStore,
      title: Strings.Wallet.enterPasswordForBiometricsNavTitle,
      message: Strings.Wallet.enterPasswordForBiometricsTitle,
      action: { password, completion in
        keyringStore.validate(password: password) { isValid in
          if isValid {
            // store password in keychain
            if case let status = keyringStore.storePasswordInKeychain(password),
               status != errSecSuccess {
              completion(.incorrectPassword)
            } else {
              // password stored in keychain
              completion(nil)
            }
          } else {
            // Conflict with the keyboard submit/dismissal that causes a bug
            // with SwiftUI animating the screen away...
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.05) {
              completion(.init(message: Strings.Wallet.incorrectPasswordErrorMessage))
            }
          }
        }
      })
  }
}

#if DEBUG
struct BiometricsPasscodeEntryView_Previews: PreviewProvider {
  static var previews: some View {
    BiometricsPasscodeEntryView(
      keyringStore: .previewStore
    )
  }
}
#endif
