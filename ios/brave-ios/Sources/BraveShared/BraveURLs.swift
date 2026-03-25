// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import UIKit

extension URL {
  public enum Brave {
    public static let community = URL(string: "https://community.brave.app/")!
    public static let account = URL(string: "https://account.brave.com")!
    public static let privacy = URL(string: "https://brave.com/privacy/")!
    public static let braveNews = URL(string: "https://brave.com/brave-news/")!
    public static let braveNewsPrivacy = URL(string: "https://brave.com/privacy/#brave-news")!
    public static let braveOffers = URL(string: "https://offers.brave.com/")!
    public static let playlist = URL(string: "https://brave.com/playlist")!
    public static let rewardsOniOS = URL(string: "https://brave.com/rewards-ios/")!
    public static let rewardsUnverifiedPublisherLearnMoreURL = URL(
      string: "https://brave.com/faq-rewards/#unclaimed-funds"
    )!
    public static let termsOfUse = URL(string: "https://www.brave.com/terms_of_use")!
    public static let batTermsOfUse = URL(
      string: "https://basicattentiontoken.org/user-terms-of-service/"
    )!
    public static let ntpTutorialPage = URL(string: "https://brave.com/ja/ntp-tutorial")!
    public static let privacyFeatures = URL(string: "https://brave.com/privacy-features/")!
    public static let support = URL(string: "https://support.brave.app")!
    public static let p3aHelpArticle = URL(
      string: "https://support.brave.app/hc/en-us/articles/9140465918093-What-is-P3A-in-Brave-"
    )!
    public static let braveVPNFaq = URL(
      string: "https://support.brave.app/hc/en-us/articles/360045045952"
    )!
    public static let braveVPNLinkReceiptProd =
      BraveIOSAccountMatomo.accountURLWithMatomoAttribution(
        "https://account.brave.com/?intent=connect-receipt&product=vpn",
        campaign: BraveIOSAccountMatomo.Campaign.deviceLinking,
        medium: BraveIOSAccountMatomo.Medium.deviceLinking
      )
    public static let braveVPNLinkReceiptStaging =
      BraveIOSAccountMatomo.accountURLWithMatomoAttribution(
        "https://account.bravesoftware.com/?intent=connect-receipt&product=vpn",
        campaign: BraveIOSAccountMatomo.Campaign.deviceLinking,
        medium: BraveIOSAccountMatomo.Medium.deviceLinking
      )
    public static let braveVPNLinkReceiptDev =
      BraveIOSAccountMatomo.accountURLWithMatomoAttribution(
        "https://account.brave.software/?intent=connect-receipt&product=vpn",
        campaign: BraveIOSAccountMatomo.Campaign.deviceLinking,
        medium: BraveIOSAccountMatomo.Medium.deviceLinking
      )
    public static let braveVPNRefreshCredentials =
      BraveIOSAccountMatomo.accountURLWithMatomoAttribution(
        "https://account.brave.com/?intent=recover&product=vpn&ux=mobile",
        campaign: BraveIOSAccountMatomo.Campaign.credentialsRefresh,
        medium: BraveIOSAccountMatomo.Medium.credentialsRefresh
      )
    public static let braveVPNCheckoutURL = BraveIOSAccountMatomo.accountURLWithMatomoAttribution(
      "https://account.brave.com/?intent=checkout&product=vpn",
      campaign: BraveIOSAccountMatomo.Campaign.inAppPurchase,
      medium: BraveIOSAccountMatomo.Medium.inAppPurchase
    )
    public static let braveVPNLearnMoreURL = URL(
      string: "https://brave.com/firewall-vpn/"
    )!
    public static let safeBrowsingHelp = URL(
      string: "https://support.brave.app/hc/en-us/articles/15222663599629-Safe-Browsing-in-Brave"
    )!
    public static let screenTimeHelp = URL(
      string: "https://support.apple.com/guide/security/secd8831e732/web"
    )!
    public static let braveLeoManageSubscriptionProd =
      BraveIOSAccountMatomo.accountURLWithMatomoAttribution(
        "https://account.brave.com/plans",
        campaign: BraveIOSAccountMatomo.Campaign.inAppPurchase,
        medium: BraveIOSAccountMatomo.Medium.managePlans
      )
    public static let braveLeoManageSubscriptionStaging =
      BraveIOSAccountMatomo.accountURLWithMatomoAttribution(
        "https://account.bravesoftware.com/plans",
        campaign: BraveIOSAccountMatomo.Campaign.inAppPurchase,
        medium: BraveIOSAccountMatomo.Medium.managePlans
      )
    public static let braveLeoManageSubscriptionDev =
      BraveIOSAccountMatomo.accountURLWithMatomoAttribution(
        "https://account.brave.software/plans",
        campaign: BraveIOSAccountMatomo.Campaign.inAppPurchase,
        medium: BraveIOSAccountMatomo.Medium.managePlans
      )
    public static let braveLeoCheckoutURL = BraveIOSAccountMatomo.accountURLWithMatomoAttribution(
      "https://account.brave.com/?intent=checkout&product=leo",
      campaign: BraveIOSAccountMatomo.Campaign.inAppPurchase,
      medium: BraveIOSAccountMatomo.Medium.inAppPurchase
    )
    public static let braveLeoLinkReceiptProd =
      BraveIOSAccountMatomo.accountURLWithMatomoAttribution(
        "https://account.brave.com/?intent=link-order&product=leo",
        campaign: BraveIOSAccountMatomo.Campaign.deviceLinking,
        medium: BraveIOSAccountMatomo.Medium.deviceLinking
      )
    public static let braveLeoLinkReceiptStaging =
      BraveIOSAccountMatomo.accountURLWithMatomoAttribution(
        "https://account.bravesoftware.com/?intent=link-order&product=leo",
        campaign: BraveIOSAccountMatomo.Campaign.deviceLinking,
        medium: BraveIOSAccountMatomo.Medium.deviceLinking
      )
    public static let braveLeoLinkReceiptDev =
      BraveIOSAccountMatomo.accountURLWithMatomoAttribution(
        "https://account.brave.software/?intent=link-order&product=leo",
        campaign: BraveIOSAccountMatomo.Campaign.deviceLinking,
        medium: BraveIOSAccountMatomo.Medium.deviceLinking
      )
    public static let braveLeoRefreshCredentials =
      BraveIOSAccountMatomo.accountURLWithMatomoAttribution(
        "https://account.brave.com/?intent=recover&product=leo&ux=mobile",
        campaign: BraveIOSAccountMatomo.Campaign.credentialsRefresh,
        medium: BraveIOSAccountMatomo.Medium.credentialsRefresh
      )
    public static let braveLeoModelCategorySupport = URL(
      string:
        "https://support.brave.app/hc/en-us/articles/26727364100493-What-are-the-differences-between-Leo-s-AI-Models"
    )!
    public static let braveLeoPrivacyFeedbackLearnMoreLinkUrl = URL(
      string:
        "https://brave.com/privacy/browser/#your-feedback"
    )!
    public static let braveVPNSmartProxySupport = URL(
      string:
        "https://support.brave.app/hc/en-us/articles/32105253704333-What-is-Smart-Proxy-Routing"
    )!
    public static let braveVPNKillSwitchSupport = URL(
      string:
        "https://support.brave.app/hc/en-us/articles/32389914657549-What-is-the-Brave-VPN-Kill-Switch"
    )!
    public static let newTabTakeoverLearnMoreLinkUrl = URL(
      string:
        "https://support.brave.app/hc/en-us/articles/35182999599501"
    )!
    public static let surveyPanelistLearnMoreLinkUrl = URL(
      string:
        "https://support.brave.app/hc/en-us/articles/36550092449165"
    )!
  }
  public enum Apple {
    public static let manageSubscriptions = URL(
      string: "https://apps.apple.com/account/subscriptions"
    )

    public static let dataImportSupport = URL(
      string: "https://support.apple.com/en-ca/guide/iphone/iph1852764a6/18.0/ios/18.0"
    )!
  }
  public enum WebUI {
    public static let aiChat = URL(string: "brave://leo-ai")!

    public enum Wallet {
      public static let home = URL(string: "brave://wallet/crypto/portfolio/assets")!
      public static let buy = URL(string: "brave://wallet/crypto/fund-wallet")!
      public static let send = URL(string: "brave://wallet/send")!
      public static let swap = URL(string: "brave://wallet/swap")!
      public static let deposit = URL(string: "brave://wallet/crypto/deposit-funds")!
      public static let activity = URL(string: "brave://wallet/crypto/portfolio/activity")!
    }
    public static let wallet = Wallet.self
  }
  public static let brave = Brave.self
  public static let apple = Apple.self
  public static let webUI = WebUI.self
}

public struct AppURLScheme {
  /// The apps URL scheme for the current build channel
  public static var appURLScheme: String {
    Bundle.main.infoDictionary?["BRAVE_URL_SCHEME"] as? String ?? "brave"
  }
}
