// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

extension NumberFormatter {
  /// Currency number formatter in USD
  static var usdCurrencyFormatter: NumberFormatter {
    NumberFormatter().then {
      $0.numberStyle = .currency
      $0.currencyCode = CurrencyCode.usd.code
    }
  }
  
  func coinMarketPriceString(from value: Double) -> String? {
    assert(numberStyle == .currency)
    var decimals: Int?
    let dp = value.decimalPlaces
    if dp < 2 || value >= Double(10) {
      decimals = 2
    } else if value >= Double(1) {
      decimals = 3
    }
    minimumFractionDigits = decimals ?? 0
    maximumFractionDigits = decimals ?? 15
    
    return string(from: NSNumber(value: value))
  }
}
