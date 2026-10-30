// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Strings
import SwiftUI

func eTldPlusOneRange(in origin: String, eTldPlusOne: String) -> Range<String.Index>? {
  guard !eTldPlusOne.isEmpty,
    let hostRange = URLComponents(string: origin)?.rangeOfHost
  else {
    return nil
  }

  let host = origin[hostRange]
  let hostEnd = host.last == "." ? host.index(before: host.endIndex) : host.endIndex
  guard host.distance(from: host.startIndex, to: hostEnd) >= eTldPlusOne.count else {
    return nil
  }

  let eTldStart = host.index(hostEnd, offsetBy: -eTldPlusOne.count)
  let eTldRange = eTldStart..<hostEnd
  guard String(host[eTldRange]).caseInsensitiveCompare(eTldPlusOne) == .orderedSame,
    eTldStart == host.startIndex || host[host.index(before: eTldStart)] == "."
  else {
    return nil
  }

  return eTldRange
}

private func originText(_ origin: String, eTldPlusOne: String) -> Text {
  guard let range = eTldPlusOneRange(in: origin, eTldPlusOne: eTldPlusOne) else {
    return Text(origin)
  }

  let originStart = origin[origin.startIndex..<range.lowerBound]
  let etldPlusOne = origin[range]
  let originEnd = origin[range.upperBound...]
  return
    Text(String(originStart).zwspOutput) + Text(etldPlusOne).bold()
    + Text(String(originEnd).zwspOutput)
}

extension Text {
  /// Creates a text view that displays a URLOrigin, bolding the eTLD+1.
  init(urlOrigin: URLOrigin) {
    let origin = urlOrigin.url?.absoluteString ?? ""
    let eTldPlusOne = urlOrigin.url?.baseDomain ?? ""
    self = originText(origin, eTldPlusOne: eTldPlusOne)
  }

  /// Creates a text view that displays a BraveWallet.OriginInfo, bolding the eTLD+1.
  init(originInfo: BraveWallet.OriginInfo?) {
    if let originInfo = originInfo {
      self = originText(originInfo.originSpec, eTldPlusOne: originInfo.eTldPlusOne)
    } else {
      self = Text(Strings.Wallet.braveWallet)
    }
  }
}
