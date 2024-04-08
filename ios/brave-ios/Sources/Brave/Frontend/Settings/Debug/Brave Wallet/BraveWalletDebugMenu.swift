// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import SwiftUI
import BraveWallet
import Preferences

struct BraveWalletDebugMenu: View {

  @ObservedObject var enableBitcoinTestnet = Preferences.Wallet.isBitcoinTestnetEnabled

  var body: some View {
    List {
      Section {
        Toggle("Enable Bitcoin Testnet", isOn: $enableBitcoinTestnet.value)
          .toggleStyle(SwitchToggleStyle(tint: .accentColor))
      }
    }
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
  }
}
