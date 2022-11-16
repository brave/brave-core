/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import UIKit

public struct BraveUX {
  public static let braveCommunityURL = URL(string: "https://community.brave.com/")!
  public static let braveVPNFaqURL = URL(string: "https://support.brave.com/hc/en-us/articles/360045045952")!
  public static let braveVPNLinkReceiptProd =
    URL(string: "https://account.brave.com/?intent=connect-receipt&product=vpn")!
  public static let braveVPNLinkReceiptStaging =
    URL(string: "https://account.bravesoftware.com/?intent=connect-receipt&product=vpn")!
  public static let braveVPNLinkReceiptDev =
    URL(string: "https://account.brave.software/?intent=connect-receipt&product=vpn")!
  public static let bravePrivacyURL = URL(string: "https://brave.com/privacy/")!
  public static let braveNewsPrivacyURL = URL(string: "https://brave.com/privacy/#brave-news")!
  public static let braveOffersURL = URL(string: "https://offers.brave.com/")!
  public static let bravePlaylistOnboardingURL = URL(string: "https://brave.com/playlist")!
  public static let braveRewardsLearnMoreURL = URL(string: "https://brave.com/rewards-ios/")!
  public static let braveRewardsUnverifiedPublisherLearnMoreURL = URL(string: "https://brave.com/faq-rewards/#unclaimed-funds")!
  public static let braveNewsPartnersURL = URL(string: "https://brave.com/brave-news/")!
  public static let braveTermsOfUseURL = URL(string: "https://www.brave.com/terms_of_use")!
  public static let batTermsOfUseURL = URL(string: "https://basicattentiontoken.org/user-terms-of-service/")!
  public static let ntpTutorialPageURL = URL(string: "https://brave.com/ja/ntp-tutorial")
  public static let privacyReportsURL = URL(string: "https://brave.com/privacy-features/")!
  public static let braveWalletNetworkLearnMoreURL = URL(string: "https://support.brave.com")!
  public static let braveP3ALearnMoreURL = URL(string: "https://support.brave.com/hc/en-us/articles/9140465918093-What-is-P3A-in-Brave-")!

  public static let faviconBorderColor = UIColor(white: 0, alpha: 0.2)
  public static let faviconBorderWidth = 1.0 / UIScreen.main.scale
  public static let baseDimensionValue = 450.0
}
