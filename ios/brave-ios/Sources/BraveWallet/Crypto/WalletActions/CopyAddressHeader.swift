// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import DesignSystem
import Strings
import SwiftUI

/// List header for displaying text with `...` menu to copy address.
/// Bitcoin accounts will only display `...` menu when `btcAccountInfo`
/// is populated, allowing copy of `nextReceiveAddress`.
struct CopyAddressHeader: View {

  let displayText: String
  let account: BraveWallet.AccountInfo
  let btcAccountInfo: BraveWallet.BitcoinAccountInfo?

  init(
    displayText: String,
    account: BraveWallet.AccountInfo,
    btcAccountInfo: BraveWallet.BitcoinAccountInfo?
  ) {
    self.displayText = displayText
    self.account = account
    self.btcAccountInfo = btcAccountInfo
  }

  var body: some View {
    HStack {
      Text(displayText)
      Spacer()
      if account.coin == .btc {
        if let btcAccountInfo {
          addressMenu(for: btcAccountInfo.nextReceiveAddress.addressString)
        }
      } else {
        addressMenu(for: account.address)
      }
    }
  }

  private func addressMenu(for addressString: String) -> some View {
    Menu(
      content: {
        Text(addressString.zwspOutput)
        Button {
          UIPasteboard.general.string = addressString
        } label: {
          Label(
            Strings.Wallet.copyAddressButtonTitle,
            braveSystemImage: "leo.copy.plain-text"
          )
        }
      },
      label: {
        Image(braveSystemName: "leo.more.horizontal")
          .padding(6)
          .clipShape(Rectangle())
      }
    )
  }
}
