// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import UIKit

extension URL {
  public enum Brave {
    public static let community = URL(string: "https://community.brave.com/")!
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
    public static let support = URL(string: "https://support.brave.com")!
    public static let p3aHelpArticle = URL(
      string: "https://support.brave.com/hc/en-us/articles/9140465918093-What-is-P3A-in-Brave-"
    )!
    public static let braveVPNFaq = URL(
      string: "https://support.brave.com/hc/en-us/articles/360045045952"
    )!
    public static let braveVPNLinkReceiptProd = URL(
      string: "https://account.brave.com/?intent=connect-receipt&product=vpn"
    )!
    public static let braveVPNLinkReceiptStaging = URL(
      string: "https://account.bravesoftware.com/?intent=connect-receipt&product=vpn"
    )!
    public static let braveVPNLinkReceiptDev = URL(
      string: "https://account.brave.software/?intent=connect-receipt&product=vpn"
    )!
    public static let braveVPNRefreshCredentials = URL(
      string: "https://account.brave.com/?intent=recover&product=vpn&ux=mobile"
    )!
    public static let safeBrowsingHelp = URL(
      string: "https://support.brave.com/hc/en-us/articles/15222663599629-Safe-Browsing-in-Brave"
    )!
    public static let screenTimeHelp = URL(
      string: "https://support.apple.com/guide/security/secd8831e732/web"
    )!
    public static let braveLeoManageSubscriptionProd = URL(
      string: "https://account.brave.com/plans"
    )!
    public static let braveLeoManageSubscriptionStaging = URL(
      string: "https://account.bravesoftware.com/plans"
    )!
    public static let braveLeoManageSubscriptionDev = URL(
      string: "https://account.brave.software/plans"
    )!
    public static let braveLeoLinkReceiptProd = URL(
      string: "https://account.brave.com/?intent=link-order&product=leo"
    )!
    public static let braveLeoLinkReceiptStaging = URL(
      string: "https://account.bravesoftware.com/?intent=link-order&product=leo"
    )!
    public static let braveLeoLinkReceiptDev = URL(
      string: "https://account.brave.software/?intent=link-order&product=leo"
    )!
    public static let braveLeoRefreshCredentials = URL(
      string: "https://account.brave.com/?intent=recover&product=leo&ux=mobile"
    )!
    public static let braveLeoModelCategorySupport = URL(
      string: "https://support.brave.com/hc/en-us/articles/26727364100493-What-are-the-differences-between-Leo-s-AI-Models"
    )!
    public static let braveVPNSmartProxySupport = URL(
      string:
        "https://support.brave.com/hc/en-us/articles/32105253704333-What-is-Smart-Proxy-Routing"
    )!
    public static let braveVPNKillSwitchSupport = URL(
      string:
        "https://support.brave.com/hc/en-us/articles/32389914657549-What-is-the-Brave-VPN-Kill-Switch"
    )!
  }
  public enum Apple {
    public static let manageSubscriptions = URL(
      string: "https://apps.apple.com/account/subscriptions"
    )
  }
  public static let brave = Brave.self
  public static let apple = Apple.self
}

public struct AppURLScheme {
  /// The apps URL scheme for the current build channel
  public static var appURLScheme: String {
    Bundle.main.infoDictionary?["BRAVE_URL_SCHEME"] as? String ?? "brave"
  }
}
