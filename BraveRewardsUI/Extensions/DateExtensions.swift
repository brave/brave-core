/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation

extension Date {
  /// Returns localized name of current month, using Gregorian calendar format.
  func currentMonthName(for locale: Locale = Locale.current) -> String {
    let calendar = Calendar(identifier: .gregorian)
    let monthNumber = calendar.component(.month, from: self)
    let dateFormatter = DateFormatter()
    dateFormatter.locale = locale
    
    return dateFormatter.standaloneMonthSymbols[monthNumber - 1]
  }
  
  /// Returns current year using Gregorian calendar format.
  var currentYear: Int {
    let calendar = Calendar(identifier: .gregorian)
    return calendar.component(.year, from: self)
  }
  
  /// Returns number of current month using Gregorian calendar format.
  /// Months numbering starts from 1(January)
  var currentMonthNumber: Int {
    let calendar = Calendar(identifier: .gregorian)
    return calendar.component(.month, from: self)
  }
  
  static func stringFrom(reconcileStamp: UInt64) -> String {
    let dateFormatter = DateFormatter().then {
      $0.dateStyle = .short
      $0.timeStyle = .none
    }
    let reconcileDate = Date(timeIntervalSince1970: TimeInterval(integerLiteral: Int64(reconcileStamp)))
    return dateFormatter.string(from: reconcileDate)
  }
}
