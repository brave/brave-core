// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import StoreKit
import Shared
import os.log

public class VPNProductInfo: NSObject {
  // Prices are fetched once per launch and kept in memory.
  // If the prices could not be fetched, we retry after user tries to go to buy-vpn screen.
  static var monthlySubProduct: SKProduct?
  static var yearlySubProduct: SKProduct?

  /// Whether we have enough product info to present to the user.
  /// If the user has bought the vpn already, it returns `true` since we do not need price details anymore.
  public static var isComplete: Bool {
    switch BraveVPN.vpnState {
    case .purchased:
      return true
    case .notPurchased, .expired:
      guard let monthlyPlan = monthlySubProduct, let yearlyPlan = yearlySubProduct else {
        return false
      }

      // Make sure the price can be displayed correctly.
      return monthlyPlan.price.frontSymbolCurrencyFormatted(with: monthlyPlan.priceLocale) != nil
        && yearlyPlan.price.frontSymbolCurrencyFormatted(with: yearlyPlan.priceLocale) != nil
    }
  }

  private let productRequest: SKProductsRequest

  /// These product ids work only on release channel.
  struct ProductIdentifiers {
    /// Apple's monthly IAP
    static let monthlySub = "bravevpn.monthly"
    /// Apple's yearly IAP
    static let yearlySub = "bravevpn.yearly"
    /// account.brave.com  monthly subscription product
    static let monthlySubSKU = "brave-premium"

    static let all = Set<String>(arrayLiteral: monthlySub, yearlySub)
  }

  public override init() {
    productRequest = SKProductsRequest(productIdentifiers: ProductIdentifiers.all)
    super.init()
    productRequest.delegate = self
  }

  public func load() {
    productRequest.start()
  }
}

extension VPNProductInfo: SKProductsRequestDelegate {
  public func productsRequest(_ request: SKProductsRequest, didReceive response: SKProductsResponse) {
    response.products.forEach {
      switch $0.productIdentifier {
      case ProductIdentifiers.monthlySub:
        VPNProductInfo.monthlySubProduct = $0
      case ProductIdentifiers.yearlySub:
        VPNProductInfo.yearlySubProduct = $0
      default:
        assertionFailure("Found product identifier that doesn't match")
      }
    }
  }

  public func request(_ request: SKRequest, didFailWithError error: Error) {
    Logger.module.error("SKProductsRequestDelegate error: \(error.localizedDescription)")
  }
}
