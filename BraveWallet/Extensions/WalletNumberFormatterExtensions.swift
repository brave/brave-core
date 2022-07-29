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
}
