// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Strings

extension Strings {
  public struct Origin {
    public static let originProductName = "Origin"  // This is not translated
    public static let adsHeader = NSLocalizedString(
      "adsHeader",
      bundle: .module,
      value: "Ads",
      comment: "A section header title for ads-related settings"
    )
    public static let rewardsLabel = NSLocalizedString(
      "rewardsLabel",
      bundle: .module,
      value: "Rewards",
      comment: "A toggle label for Brave Rewards feature"
    )
    public static let analyticsHeader = NSLocalizedString(
      "analyticsHeader",
      bundle: .module,
      value: "Analytics",
      comment: "A section header title for analytics-related settings"
    )
    public static let privacyPreservingAnalyticsLabel = NSLocalizedString(
      "privacyPreservingAnalyticsLabel",
      bundle: .module,
      value: "Privacy preserving analytics",
      comment: "A toggle label for privacy preserving analytics feature"
    )
    public static let statisticsReportingLabel = NSLocalizedString(
      "statisticsReportingLabel",
      bundle: .module,
      value: "Statistics reporting",
      comment: "A toggle label for statistics reporting feature"
    )
    public static let featuresHeader = NSLocalizedString(
      "featuresHeader",
      bundle: .module,
      value: "Features",
      comment: "A section header title that is shown over a list of Brave specific features"
    )
    public static let leoAILabel = NSLocalizedString(
      "leoAILabel",
      bundle: .module,
      value: "Leo AI",
      comment: "A toggle label for Brave Leo AI feature"
    )
    public static let newsLabel = NSLocalizedString(
      "newsLabel",
      bundle: .module,
      value: "News",
      comment: "A toggle label for Brave News feature"
    )
    public static let playlistLabel = NSLocalizedString(
      "playlistLabel",
      bundle: .module,
      value: "Playlist",
      comment: "A toggle label for Brave Playlist feature"
    )
    public static let talkLabel = NSLocalizedString(
      "talkLabel",
      bundle: .module,
      value: "Talk",
      comment: "A toggle label for Brave Talk feature"
    )
    public static let vpnLabel = NSLocalizedString(
      "vpnLabel",
      bundle: .module,
      value: "VPN",
      comment: "A toggle label for Brave VPN feature"
    )
    public static let walletLabel = NSLocalizedString(
      "walletLabel",
      bundle: .module,
      value: "Wallet",
      comment: "A toggle label for Brave Wallet feature"
    )
    public static let featuresFooter = NSLocalizedString(
      "featuresFooter",
      bundle: .module,
      value: """
        Brave Origin is a subscription-based way to disable the revenue-generating features that otherwise support Brave as a business. Brave Origin subscribers will continue to benefit from our industry-leading privacy, adblock, and speed (via Brave Shields), as well as regular software updates, Chromium patches, and security and privacy improvements. They'll also unlock the option to disable features like Brave Leo, News, Rewards, Wallet, VPN, Web3 domains, and more.

        Subscribers will see a new control panel in the browser Settings menu. Existing features—as well as any new features we ship in the future—would appear here, and be toggled off by default.

        **Note: Brave Origin will not disable browser features that you currently use, and will not affect Brave Search features or Brave Search Ads.**
        """,
      comment:
        "A footer text explaining Brave Origin subscription and its features. The stars represent markdown and the same paragraph should be also be bolded when translated."
    )
    public static let resetToDefaultsButton = NSLocalizedString(
      "resetToDefaultsButton",
      bundle: .module,
      value: "Reset to Defaults",
      comment: "A button label to reset all Origin settings to their default values"
    )
  }
}
