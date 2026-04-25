// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Strings
import SwiftUI

extension Text {
  /// Creates a text view that displays a URLOrigin, bolding the eTLD+1.
  init(urlOrigin: URLOrigin) {
    if urlOrigin == WalletConstants.braveWalletOrigin {
      self = Text(Strings.Wallet.braveWallet)
    } else {
      let origin = urlOrigin.url?.absoluteString ?? ""
      let eTldPlusOne = urlOrigin.url?.baseDomain ?? ""
      if let range = origin.range(of: eTldPlusOne) {
        let originStart = origin[origin.startIndex..<range.lowerBound]
        let etldPlusOne = origin[range.lowerBound..<range.upperBound]
        let originEnd = origin[range.upperBound...]
        self =
          Text(String(originStart).zwspOutput) + Text(etldPlusOne).bold()
          + Text(String(originEnd).zwspOutput)
      } else {
        self = Text(origin)
      }
    }
  }

  /// Creates a text view that displays a BraveWallet.OriginInfo, bolding the eTLD+1.
  init(originInfo: BraveWallet.OriginInfo) {
    // Internal Transaction from Brave:
    // originInfo.eTldPlusOne=""
    // originInfo.originSpec="chrome://wallet"
    // From Uniswap:
    // originInfo.eTldPlusOne="uniswap.org"
    // originInfo.originSpec="https://app.uniswap.org"
    if originInfo.isBraveWalletOrigin {
      self = Text(Strings.Wallet.braveWallet)
    } else {
      let origin = originInfo.originSpec
      let eTldPlusOne = originInfo.eTldPlusOne
      if let range = origin.range(of: eTldPlusOne) {
        let originStart = origin[origin.startIndex..<range.lowerBound]
        let etldPlusOne = origin[range.lowerBound..<range.upperBound]
        let originEnd = origin[range.upperBound...]
        self =
          Text(String(originStart).zwspOutput) + Text(etldPlusOne).bold()
          + Text(String(originEnd).zwspOutput)
      } else {
        self = Text(origin)
      }
    }
  }
}
