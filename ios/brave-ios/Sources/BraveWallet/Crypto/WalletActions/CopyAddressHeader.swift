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

  enum AccountInfo {
    case regular(_ accountInfo: BraveWallet.AccountInfo)
    case btc(_ accountInfo: BraveWallet.BitcoinAccountInfo)
    case zec(_ accountInfo: BraveWallet.ZCashAccountInfo)
  }
  let displayText: String
  let accountInfo: AccountInfo

  init(
    displayText: String,
    accountInfo: AccountInfo
  ) {
    self.displayText = displayText
    self.accountInfo = accountInfo
  }

  var body: some View {
    HStack {
      Text(displayText)
      Spacer()
      switch accountInfo {
      case .regular(let accountInfo):
        addressMenu(for: accountInfo.address)
      case .btc(let btcAccountInfo):
        addressMenu(for: btcAccountInfo.nextReceiveAddress.addressString)
      case .zec(let zecAccountInfo):
        addressMenu(for: zecAccountInfo.nextTransparentReceiveAddress.addressString)
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
