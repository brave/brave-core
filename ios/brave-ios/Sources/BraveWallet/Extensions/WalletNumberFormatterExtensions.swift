// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

extension NumberFormatter {
  /// Currency number formatter in USD
  static var usdCurrencyFormatter: NumberFormatter {
    NumberFormatter().then {
      $0.numberStyle = .currency
      $0.currencyCode = CurrencyCode.usd.code
    }
  }

  /// Formats the given `value` for fiat display.
  /// Values less than 1 will show up to 3 significant digits.
  /// Values greater than 1 & less than 10 will show 3 digits after the decimal.
  /// All other values will display with grouping and 2 digits after the decimal.
  func formatAsFiat(_ value: Double) -> String? {
    let currencyFormatter = NumberFormatter.usdCurrencyFormatter
    let prevUsesSignificantDigits = currencyFormatter.usesSignificantDigits
    let prevMaxSignificantDigits = currencyFormatter.maximumSignificantDigits
    let prevMinFractionDigits = currencyFormatter.minimumFractionDigits
    let prevMaxFractionDigits = currencyFormatter.maximumFractionDigits
    let valueDecimalPlaces = value.decimalPlaces
    if abs(value) < 1 && valueDecimalPlaces != 0 {
      currencyFormatter.usesSignificantDigits = true
      currencyFormatter.maximumSignificantDigits = 3
    } else if abs(value) < 10 && valueDecimalPlaces != 0 {
      currencyFormatter.minimumFractionDigits = 2
      currencyFormatter.maximumFractionDigits = 3
    }
    let result = currencyFormatter.string(from: .init(value: value))
    currencyFormatter.usesSignificantDigits = prevUsesSignificantDigits
    currencyFormatter.maximumSignificantDigits = prevMaxSignificantDigits
    currencyFormatter.minimumFractionDigits = prevMinFractionDigits
    currencyFormatter.maximumFractionDigits = prevMaxFractionDigits
    return result
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
