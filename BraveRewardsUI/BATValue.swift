/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation

/// Ledger uses different types to represent BAT values.
/// This structure helps parsing and converting it.
public struct BATValue: DisplayableOption {
  private let value: Double
  
  static let probiMultiplier = pow(10, 18) as NSDecimalNumber
  
  public init?(_ stringValue: String) {
    guard let doubleFromString = Double(stringValue) else { return nil }
    value = doubleFromString
  }
  
  public init(_ doubleValue: Double) {
    value = doubleValue
  }
  
  public init?(probi probiValue: String, precision: Int16 = 3) {
    let decimalNumber = NSDecimalNumber(string: probiValue)
    
    if decimalNumber == NSDecimalNumber.notANumber { return nil }
    
    // probi should be rounded down
    let dividingBehavior = NSDecimalNumberHandler(roundingMode: .down, scale: precision,
                                                  raiseOnExactness: false, raiseOnOverflow: false,
                                                  raiseOnUnderflow: false, raiseOnDivideByZero: false)
    
    value = decimalNumber.dividing(by: BATValue.probiMultiplier, withBehavior: dividingBehavior).doubleValue
  }
  
  var doubleValue: Double {
    return value
  }
  
  public var displayString: String {
    return String(format: "%.3f", value)
  }
  
  var probiValue: String {
    let decimalNumber = NSDecimalNumber(value: value)
    return decimalNumber.multiplying(by: BATValue.probiMultiplier).stringValue
  }
}
