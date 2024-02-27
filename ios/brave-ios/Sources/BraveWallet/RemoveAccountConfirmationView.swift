// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import DesignSystem
import Strings
import SwiftUI

struct RemoveAccountConfirmationView: View {

  let account: BraveWallet.AccountInfo
  var keyringStore: KeyringStore

  var body: some View {
    PasswordEntryView(
      keyringStore: keyringStore,
      message: String.localizedStringWithFormat(
        Strings.Wallet.removeAccountConfirmationMessage,
        account.name
      ),
      action: { password, completion in
        keyringStore.removeSecondaryAccount(
          for: account,
          password: password,
          completion: { success in
            completion(success ? nil : .incorrectPassword)
          }
        )
      }
    )
  }
}

#if DEBUG
struct RemoveAccountConfirmationView_Previews: PreviewProvider {
  static var previews: some View {
    RemoveAccountConfirmationView(
      account: .previewAccount,
      keyringStore: .previewStore
    )
  }
}
#endif
