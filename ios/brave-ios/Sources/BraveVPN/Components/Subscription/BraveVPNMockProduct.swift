// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import StoreKit

class BraveVPNMockSKProduct: SKProduct {
  private let mockProductIdentifier: String
  private let mockLocalizedTitle: String
  private let mockLocalizedDescription: String
  private let mockPrice: NSDecimalNumber
  private let mockPriceLocale: Locale

  init(price: NSDecimalNumber, priceLocale: Locale) {
    self.mockProductIdentifier = "bravevpn"
    self.mockLocalizedTitle = ""
    self.mockLocalizedDescription = ""
    self.mockPrice = price
    self.mockPriceLocale = priceLocale

    super.init()
  }

  override var productIdentifier: String {
    return mockProductIdentifier
  }

  override var localizedTitle: String {
    return mockLocalizedTitle
  }

  override var localizedDescription: String {
    return mockLocalizedDescription
  }

  override var price: NSDecimalNumber {
    return mockPrice
  }

  override var priceLocale: Locale {
    return mockPriceLocale
  }
}
