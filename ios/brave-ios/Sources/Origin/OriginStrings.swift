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
    public static let enabledFeatureNote = NSLocalizedString(
      "enabledFeatureNote",
      bundle: .module,
      value: "This feature is enabled because you opted into it",
      comment: "A note that indicates a feature is enabled"
    )
    public static let featuresFooter = NSLocalizedString(
      "featuresFooter",
      bundle: .module,
      value: """
        Brave Origin is a version of the Brave browser that allows you to easily disable the revenue-generating features that otherwise support Brave as a business. Brave Origin users will continue to benefit from our industry-leading privacy, adblock, and speed (via Brave Shields), as well as regular software updates, Chromium patches, and security and privacy improvements. They’ll also unlock the option to disable features like Brave Leo, Rewards, Wallet, VPN, Web3 domains, and more.

        Once purchased, users will see a new control panel in the browser Settings menu. Existing features—as well as any new features we ship in the future—would appear here, and be toggled off by default.

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
    public static let paywallDescription = NSLocalizedString(
      "paywallDescription",
      bundle: .module,
      value: """
        **Customize your browser while still supporting Brave**

        Brave Origin is a paid version of the browser for users who don't need all of the features that  support Brave as a business, but still want the privacy that only Brave offers. Brave Origin users will continue to benefit from our industry-leading privacy, adblock, and speed (via Brave Shields), as well as regular software updates, Chromium patches, and security and privacy improvements. They’ll also unlock the option to disable features like AI, Rewards, Wallet, VPN, Web3 domains, and more.

        **Brave Origin is available via one-time purchase. On mobile (Android and iOS) and desktop, Origin can be purchased as an upgrade to your existing release version.** On desktop, Origin is also available as a separate, standalone app. Whether purchased as an upgrade or separate app, Origin users will see a new control panel in the Settings menu. Existing features—as well as any new features we ship in the future—would appear here, and be toggled off by default.*

        * Note that if you’ve purchased Origin as an upgrade to your existing version of Brave, Origin will not disable browser features that you currently use. For both the upgrade and standalone version, Origin will not affect Brave Search features or Brave Search Ads.
        """,
      comment:
        "A description of Brave Origin displayed on the paywall screen. This text contains Markdown formatting, the stars represent markdown and the same paragraphs should be also be bolded when translated."
    )
    public static let alreadyPurchasedTitle = NSLocalizedString(
      "alreadyPurchasedTitle",
      bundle: .module,
      value: "Already purchased on brave.com?",
      comment: "A title asking if the user already purchased on brave.com"
    )
    public static let getLoginCodeButton = NSLocalizedString(
      "getLoginCodeButton",
      bundle: .module,
      value: "Get A Login Code",
      comment: "A button label to get a login code for users who purchased on brave.com"
    )
    public static let promoCodeTitle = NSLocalizedString(
      "promoCodeTitle",
      bundle: .module,
      value: "Have a promo code?",
      comment: "A title asking if the user has a promo code"
    )
    public static let redeemPromoCodeButton = NSLocalizedString(
      "redeemPromoCodeButton",
      bundle: .module,
      value: "Redeem Promo Code",
      comment: "A button label to redeem a promo code"
    )
    public static let restoreButton = NSLocalizedString(
      "restoreButton",
      bundle: .module,
      value: "Restore",
      comment: "A button label to restore previous purchases"
    )
    public static let purchaseErrorMessage = NSLocalizedString(
      "purchaseErrorMessage",
      bundle: .module,
      value:
        "Unable to complete purchase. Please try again, or check your payment details on Apple and try again.",
      comment: "An error message displayed when a purchase fails"
    )
    public static let buyNowButton = NSLocalizedString(
      "buyNowButton",
      bundle: .module,
      value: "Buy Now",
      comment: "A button label to purchase Brave Origin"
    )
    public static let upsellSupportMission = NSLocalizedString(
      "upsellSupportMission",
      bundle: .module,
      value: "Support our mission & open-source work",
      comment: "An upsell point about supporting Brave's mission and open-source work"
    )
    public static let upsellMinimalUI = NSLocalizedString(
      "upsellMinimalUI",
      bundle: .module,
      value: "Minimal browser UI centered on Shields",
      comment: "An upsell point about minimal browser UI"
    )
    public static let upsellCoreFeatures = NSLocalizedString(
      "upsellCoreFeatures",
      bundle: .module,
      value: "Maintain core adblock, privacy, & speed",
      comment: "An upsell point about maintaining core features like adblock, privacy, and speed"
    )
    public static let upsellOneTimePurchase = NSLocalizedString(
      "upsellOneTimePurchase",
      bundle: .module,
      value: "One-time purchase, no additional subscription cost",
      comment: "An upsell point about the one-time purchase model"
    )
    public static let oneTimePurchaseLabel = NSLocalizedString(
      "oneTimePurchaseLabel",
      bundle: .module,
      value: "One Time Purchase",
      comment: "A label indicating this is a one-time purchase product"
    )
  }
}
