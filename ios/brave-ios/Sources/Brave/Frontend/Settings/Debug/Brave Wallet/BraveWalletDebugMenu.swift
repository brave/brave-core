// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import BraveWallet
import Preferences
import SwiftUI

struct BraveWalletDebugMenu: View {

  @ObservedObject var enableBitcoinTestnet = Preferences.Wallet.isBitcoinTestnetEnabled
  @ObservedObject var enableZcashTestnet = Preferences.Wallet.isZcashTestnetEnabled

  var body: some View {
    Form {
      Section {
        Toggle("Enable Bitcoin Testnet", isOn: $enableBitcoinTestnet.value)
          .toggleStyle(SwitchToggleStyle(tint: .accentColor))
        Toggle("Enable Zcash Testnet", isOn: $enableZcashTestnet.value)
          .toggleStyle(SwitchToggleStyle(tint: .accentColor))
      }
    }
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
    .navigationTitle("Brave Wallet Debug")
    .navigationBarTitleDisplayMode(.inline)
  }
}
