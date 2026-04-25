// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BigNumber
import Foundation

/// Handles formatting between Wei and decimal values
struct WalletAmountFormatter {
  /// The style to use when formatting wei to decimals
  var decimalFormatStyle: DecimalFormatStyle

  enum DecimalFormatStyle {
    /// A basic decimal format with the requested precision
    ///
    /// E.g. `3.14159` if you supplied 5 as the precision
    case decimals(precision: Int)
    /// A format to be used to display a token's balance.
    ///
    /// Shows 4 decimals of precision and is rounded. Example: `3.1415`
    case balance
    /// A format to be used when you want to display a deicmal gas amount.
    ///
    /// Shows 10 decimals of precision and is rounded. Example: `3.141592`
    case gasFee(limit: String, radix: Radix = .decimal)

    fileprivate var precision: Int {
      switch self {
      case .decimals(let precision):
        return precision
      case .balance:
        return 4
      case .gasFee:
        return 10
      }
    }

    fileprivate var isRounded: Bool {
      switch self {
      case .decimals:
        return false
      case .balance, .gasFee:
        return true
      }
    }
  }

  enum Radix: Int {
    case decimal = 10
    case hex = 16
  }

  private static func isStringValid(_ string: String, radix: Radix) -> Bool {
    switch radix {
    case .decimal:
      return Double(string.removingHexPrefix) != nil
    case .hex:
      return string.allSatisfy(\.isHexDigit)
    }
  }

  /// Get a decimal representation of a value string given some decimal precision of said value
  ///
  /// - parameters:
  ///     - value: A string representation of the value, typically a wei value string.
  ///     - radix: The radix of the value string, defaults to 10
  ///     - decimals: The number of decimal precision to convert to. For example, ETH and
  ///                 ERC20 tokens will have 18 decimals by default
  func decimalString(for value: String, radix: Radix = .decimal, decimals: Int) -> String? {
    guard !value.isEmpty, Self.isStringValid(value, radix: radix),
      let bv = BDouble(value, radix: radix.rawValue)
    else {
      return nil
    }
    var decimal = bv / (BDouble(10) ** decimals)
    if case .gasFee(let limit, let limitRadix) = decimalFormatStyle,
      let gasLimit = BDouble(limit, radix: limitRadix.rawValue)
    {
      decimal *= gasLimit
    }
    return decimal.decimalExpansion(
      precisionAfterDecimalPoint: decimalFormatStyle.precision,
      rounded: decimalFormatStyle.isRounded
    )
  }

  /// Get a wei string from a decimal string
  ///
  /// - parameters:
  ///     - decimalValue: The decimal representation of some wei value such as `3.14159`
  ///     - radix: The radix you want to output the Wei string in
  ///     - decimals: The number of decimal precision to convert to. For example, ETH and
  ///                 ERC20 tokens will have 18 decimals by default
  func weiString(from decimalValue: String, radix: Radix = .decimal, decimals: Int) -> String? {
    guard let value = BDouble(decimalValue, radix: 10) else {
      return nil
    }
    let weiValue = (value * (BDouble(10) ** decimals))
    // Ensure that wei value is a whole number
    if weiValue.denominator != [1] {
      return nil
    }
    return weiValue.rounded().asString(radix: radix.rawValue)
  }

  /// Get a wei string from a decimal value
  ///
  /// - parameters:
  ///     - decimalValue: The decimal representation of some wei value such as `3.14159`
  ///     - radix: The radix you want to output the Wei string in
  ///     - decimals: The number of decimal precision to convert to. For example, ETH and
  ///                 ERC20 tokens will have 18 decimals by default
  func weiString(from decimalValue: Double, radix: Radix = .decimal, decimals: Int) -> String? {
    let weiValue = (decimalValue * (BDouble(10) ** decimals))
    // Ensure that wei value is a whole number
    if weiValue.denominator != [1] {
      return nil
    }
    return weiValue.rounded().asString(radix: radix.rawValue)
  }

  static func weiToDecimalGwei(
    _ weiString: String,
    radix: Radix
  ) -> String? {
    guard isStringValid(weiString, radix: radix),
      let value = BDouble(weiString, radix: radix.rawValue)
    else {
      return nil
    }
    let decimal = (value / (BDouble(10) ** 9))
    if decimal.denominator == [1] {
      return decimal.rounded().asString(radix: 10)
    } else {
      return decimal.decimalExpansion(precisionAfterDecimalPoint: 2, rounded: true)
    }
  }

  static func gweiToWei(
    _ gweiString: String,
    radix: Radix,
    outputRadix: Radix
  ) -> String? {
    guard isStringValid(gweiString, radix: radix),
      let gwei = BDouble(gweiString, radix: radix.rawValue)
    else {
      return nil
    }
    return (gwei * (BDouble(10) ** 9))
      .rounded().asString(radix: outputRadix.rawValue)
  }

  static func decimalToAmount(_ decimalString: String, tokenDecimals: Int) -> UInt64? {
    guard isStringValid(decimalString, radix: .decimal),
      let value = BDouble(decimalString, radix: Radix.decimal.rawValue)
    else {
      return nil
    }
    let stringValue = (value * (BDouble(10) ** tokenDecimals)).rounded().description
    return UInt64(stringValue)
  }
}

extension String {
  // Remove trailing zeros, including the decimal separator if necessary.
  // Example: 1.0000000000 becomes 1, 1122.33440000000 becomes 1122.3344
  var trimmingTrailingZeros: String {
    do {
      let regexPattern = "\\.0*$|(\\.\\d*[1-9])0+$"
      let regex = try NSRegularExpression(pattern: regexPattern, options: .caseInsensitive)
      let range = NSRange(location: 0, length: count)
      return regex.stringByReplacingMatches(in: self, options: [], range: range, withTemplate: "$1")
    } catch {
      return self
    }
  }
}

extension Double {
  var decimalPlaces: Int {
    if abs(self) == Double(Int(abs(self))) {
      return 0
    }

    let integerString = String(Int(abs(self)))
    let doubleString = String(Double(abs(self)))
    let decimalCount = doubleString.count - integerString.count - 1

    return decimalCount
  }
}
