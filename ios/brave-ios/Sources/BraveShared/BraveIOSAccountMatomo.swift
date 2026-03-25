// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

extension URL {
  /// Appends Matomo `mtm_*` query parameters for cookie-less cohort attribution on account.brave.com.
  public func appendingMatomoAttribution(campaign: String, source: String, medium: String) -> URL {
    guard var components = URLComponents(url: self, resolvingAgainstBaseURL: false) else {
      return self
    }
    var items = components.queryItems ?? []
    items.append(contentsOf: [
      URLQueryItem(name: "mtm_campaign", value: campaign),
      URLQueryItem(name: "mtm_source", value: source),
      URLQueryItem(name: "mtm_medium", value: medium),
    ])
    components.queryItems = items
    return components.url ?? self
  }
}

/// Matomo values for Brave iOS -- account.brave.com links
public enum BraveIOSAccountMatomo {
  public static let source = "ios"

  public enum Campaign {
    public static let inAppPurchase = "iap-ios"
    public static let deviceLinking = "linking-ios"
    public static let credentialsRefresh = "refresh-ios"
  }

  public enum Medium {
    public static let inAppPurchase = "iap"
    public static let deviceLinking = "linking"
    public static let credentialsRefresh = "refresh"
    public static let managePlans = "manage"
  }

  /// Parses `string` as a URL and appends `mtm_campaign`, `mtm_source` (`ios`), and `mtm_medium`.
  public static func accountURLWithMatomoAttribution(
    _ string: String,
    campaign: String,
    medium: String
  ) -> URL {
    URL(string: string)!.appendingMatomoAttribution(
      campaign: campaign,
      source: source,
      medium: medium
    )
  }
}
