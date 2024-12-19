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
    let prevUsesSignificantDigits = self.usesSignificantDigits
    let prevMaxSignificantDigits = self.maximumSignificantDigits
    let prevMinFractionDigits = self.minimumFractionDigits
    let prevMaxFractionDigits = self.maximumFractionDigits
    let valueDecimalPlaces = value.decimalPlaces
    if abs(value) < 1 && valueDecimalPlaces != 0 {
      self.usesSignificantDigits = true
      self.maximumSignificantDigits = 3
    } else if abs(value) < 10 && valueDecimalPlaces != 0 {
      self.minimumFractionDigits = 2
      self.maximumFractionDigits = 3
    }
    let result = self.string(from: .init(value: value))
    self.usesSignificantDigits = prevUsesSignificantDigits
    self.maximumSignificantDigits = prevMaxSignificantDigits
    self.minimumFractionDigits = prevMinFractionDigits
    self.maximumFractionDigits = prevMaxFractionDigits
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
