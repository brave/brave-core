/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import BraveRewards

extension BraveLedger {
  
  /// The total balance or 0 if the balance hasn't been loaded yet
  fileprivate var balanceTotal: Double {
    return balance?.total ?? 0
  }
  
  /// Get the current BAT wallet balance for display
  var balanceString: String { return BATValue(balanceTotal).displayString }
  
  /// Get the current USD wallet balance for display
  var usdBalanceString: String {
    return dollarStringForBATAmount(balanceTotal) ?? ""
  }
  
  /// Gets the dollar string for some BAT amount using rates from the users wallet with the
  /// currency code appended (i.e. "6.42 USD")
  func dollarStringForBATAmount(_ amount: Double, currencyCode: String = "USD", includeCurrencyCode: Bool = true) -> String? {
    guard let balance = balance,
          let conversionRate = balance.rates[currencyCode]?.doubleValue else {
      return nil
    }
    
    let currencyFormatter = NumberFormatter()
    currencyFormatter.currencySymbol = ""
    currencyFormatter.numberStyle = .currency
    let valueString = currencyFormatter.string(from: NSNumber(value: amount * conversionRate)) ?? "0.00"
    if includeCurrencyCode {
      return "\(valueString) \(currencyCode)"
    }
    return "\(valueString)"
  }
  
  /// Takes BAT amount as String, and returns a String converted to selected currency.
  /// Returns '0.00' if the method failed or could not cast `amountString` as `Double`
  func dollarStringForBATAmount(_ amountString: String, currencyCode: String = "USD", includeCurrencyCode: Bool = true) -> String {
    let stringToDouble = Double(amountString) ?? 0.0
    guard let string = dollarStringForBATAmount(stringToDouble, currencyCode: currencyCode, includeCurrencyCode: includeCurrencyCode) else {
      if includeCurrencyCode {
        return "0.00 USD"
      }
      return "0.00"
    }
    return string
  }
  
  /// Options around minimum visits for publisher relavancy
  enum MinimumVisitsOptions: UInt32, CaseIterable, DisplayableOption {
    case one = 1
    case five = 5
    case ten = 10
    
    var displayString: String {
      switch self {
      case .one: return Strings.MinimumVisitsChoices0
      case .five: return Strings.MinimumVisitsChoices1
      case .ten: return Strings.MinimumVisitsChoices2
      }
    }
  }
  
  /// Options around minimum page time before logging a visit (in seconds)
  enum MinimumVisitDurationOptions: UInt64, CaseIterable, DisplayableOption {
    case fiveSeconds = 5
    case eightSeconds = 8
    case oneMinute = 60
    
    var displayString: String {
      switch self {
      case .fiveSeconds: return Strings.MinimumLengthChoices0
      case .eightSeconds: return Strings.MinimumLengthChoices1
      case .oneMinute: return Strings.MinimumLengthChoices2
      }
    }
  }
  
  // MARK: -
  
  /// Creates the ledger wallet and fetches wallet properties and balances
  ///
  /// Use this is in UI instead of `createWallet` directly unless required
  public func createWalletAndFetchDetails(_ completion: @escaping (Bool) -> Void) {
    createWallet { [weak self] (error) in
      guard let self = self else { return }
      
      if let _ = error {
        completion(false)
        return
      }
      
      let group = DispatchGroup()
      var success = true
      group.enter()
      self.fetchWalletDetails { details in
        if details == nil {
          success = false
        }
        group.leave()
      }
      group.enter()
      self.fetchBalance { balance in
        if balance == nil {
          success = false
        }
        group.leave()
      }
      group.notify(queue: .main, execute: {
        completion(success)
      })
    }
  }
}

extension PublisherInfo {
  /// Temporary conveninece method for converting the raw int now in
  /// PublisherInfo, to the enum we've been using all along
  var rewardsCategory: RewardsType {
    guard let category = RewardsType(rawValue: Int(self.category)) else {
      fatalError("Could not get a rewards category from category = \(self.category)")
    }
    return category
  }
}
