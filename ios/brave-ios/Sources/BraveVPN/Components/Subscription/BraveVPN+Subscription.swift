// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import GuardianConnect
import Preferences
import Shared

extension BraveVPN {
  /// Type of the vpn subscription
  public enum SubscriptionType: Equatable {
    case monthly, yearly, other
  }

  /// Type of the active purchased vpn plan
  public static var activeSubscriptionType: SubscriptionType {
    guard let productId = Preferences.VPN.subscriptionProductId.value else {
      logAndStoreError("subscriptionName: failed to retrieve productId")
      return .other
    }

    switch productId {
    case BraveVPNProductInfo.ProductIdentifiers.monthlySub:
      return .monthly
    case BraveVPNProductInfo.ProductIdentifiers.yearlySub:
      return .yearly
    default:
      return .other
    }
  }

  /// Name of the purchased vpn plan.
  public static var subscriptionName: String {
    guard let productId = Preferences.VPN.subscriptionProductId.value else {
      logAndStoreError("subscriptionName: failed to retrieve productId")
      return ""
    }

    switch productId {
    case BraveVPNProductInfo.ProductIdentifiers.monthlySub:
      return Strings.VPN.vpnSettingsMonthlySubName
    case BraveVPNProductInfo.ProductIdentifiers.yearlySub:
      return Strings.VPN.vpnSettingsYearlySubName
    case BraveVPNProductInfo.ProductIdentifiers.monthlySubSKU:
      return Strings.VPN.vpnSettingsMonthlySubName
    default:
      assertionFailure("Can't get product id")
      return ""
    }
  }
}
