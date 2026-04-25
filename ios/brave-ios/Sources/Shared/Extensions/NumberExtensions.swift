// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

// MARK: - INT

extension Int {
  /// Returns larger numbers in K(thousands) and M(millions) friendly format.
  /// Example : 123 456 -> 123K, 3000 -> 3k, 3400 -> 3.4K
  public var kFormattedNumber: String {
    if self >= 1000 && self < 10000 {
      return String(format: "%.1fK", Double(self / 100) / 10).replacingOccurrences(
        of: ".0",
        with: ""
      )
    }

    if self >= 10000 && self < 1_000_000 {
      return "\(self/1000)K"
    }

    if self >= 1_000_000 && self < 10_000_000 {
      return String(format: "%.1fM", Double(self / 100000) / 10).replacingOccurrences(
        of: ".0",
        with: ""
      )
    }

    if self >= 10_000_000 {
      return "\(self/1_000_000)M"
    }

    return String(self)
  }
}

extension NSDecimalNumber {

  /// Returns a currency formatted string where the currency's symbol is in front.
  /// For example $19.99.
  public func frontSymbolCurrencyFormatted(
    with locale: Locale,
    isSymbolIncluded: Bool = true
  ) -> String? {
    let formatter = NumberFormatter()
    formatter.numberStyle = .currency
    formatter.locale = locale
    // Hide currency symbol, we append it manually in front of the String.
    formatter.currencySymbol = ""
    formatter.currencyCode = ""

    let currencySymbol = locale.currencySymbol ?? ""

    // Making currency codes empty adds extra space at the end, trimming it here.
    guard
      let formatted = formatter.string(from: self)?
        .trimmingCharacters(in: .whitespacesAndNewlines)
    else { return nil }

    return isSymbolIncluded ? currencySymbol + formatted : formatted
  }
}

extension Decimal {

  /// Returns a currency formatted string where the currency's symbol is in front.
  /// For example $19.99.
  public func frontSymbolCurrencyFormatted(
    with locale: Locale,
    isSymbolIncluded: Bool = true
  ) -> String? {
    return (self as NSDecimalNumber).frontSymbolCurrencyFormatted(
      with: locale,
      isSymbolIncluded: isSymbolIncluded
    )
  }
}
