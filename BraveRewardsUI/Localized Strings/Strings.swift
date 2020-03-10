/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import BraveShared

internal struct Strings {
  /// "BAT" or "BAT Points" depending on the region
  static var BAT: String {
    return Preferences.Rewards.isUsingBAP.value == true ? "BAP" : "BAT"
  }
}

internal extension Strings {
  static var walletBalanceType: String {
    if Preferences.Rewards.isUsingBAP.value == true {
      return NSLocalizedString("BATPoints", bundle: .rewardsUI, value: "BAT Points", comment: "")
    }
    return Strings.BAT
  }
  static let open = NSLocalizedString("BraveRewardsOpen", bundle: .rewardsUI, value: "Open", comment: "")
  static let adNotificationTitle = NSLocalizedString("BraveRewardsAdNotificationTitle", bundle: .rewardsUI, value: "Brave Rewards", comment: "")
  static let verified = NSLocalizedString("BraveRewardsVerified", bundle: .rewardsUI, value: "Brave Verified Creator", comment: "")
  static let checkAgain = NSLocalizedString("BraveRewardsCheckAgain", bundle: .rewardsUI, value: "Refresh Status", comment: "")
  static let rewardsOptInLearnMore = NSLocalizedString("RewardsOptInLearnMore", bundle: .rewardsUI, value: "Learn More", comment: "")
  static var settingsAdsBody: String {
    if Preferences.Rewards.isUsingBAP.value == true {
      return NSLocalizedString("BraveRewardsSettingsAdsBodyJapan", bundle: .rewardsUI, value: "Earn points by viewing ads in Brave. Ads presented are based on your interests, as inferred from your browsing behavior. No personal data or browsing history ever leaves your browser.", comment: "")
    }
    return NSLocalizedString("BraveRewardsSettingsAdsBody", bundle: .rewardsUI, value: "Earn tokens by viewing ads in Brave. Ads presented are based on your interests, as inferred from your browsing behavior. No personal data or browsing history ever leaves your browser.", comment: "")
  }
  static var walletHeaderGrants: String {
    if Preferences.Rewards.isUsingBAP.value == true {
      return NSLocalizedString("BraveRewardsWalletHeaderDetails", bundle: .rewardsUI, value: "Details", comment: "")
    }
    return NSLocalizedString("BraveRewardsWalletHeaderGrants", bundle: .rewardsUI, value: "Grants", comment: "")
  }
  static let settingsTitle = NSLocalizedString("BraveRewardsSettingsTitle", bundle: .rewardsUI, value: "Settings", comment: "")
  static let tips = NSLocalizedString("BraveRewardsTips", bundle: .rewardsUI, value: "Tips", comment: "")
  static let settingsAutoContributeTitle = NSLocalizedString("BraveRewardsSettingsAutoContributeTitle", bundle: .rewardsUI, value: "Auto-Contribute", comment: "")
  static let settingsAutoContributeUpToValue = NSLocalizedString("BraveRewardsSettingsAutoContributeUpToValue", bundle: .rewardsUI, value: "Up to %@", comment: "The maximum amount of BAT to auto-contribute. E.g. 'Up to 20.0 BAT'")
  static let notYetVerified = NSLocalizedString("BraveRewardsNotYetVerified", bundle: .rewardsUI, value: "Not yet verified", comment: "")
  static let grantsClaimedTitle = NSLocalizedString("BraveRewardsGrantsClaimedTitle", bundle: .rewardsUI, value: "It's your lucky day!", comment: "")
  static let adsGrantsClaimedTitle = NSLocalizedString("BraveRewardsAdsGrantsClaimedTitle", bundle: .rewardsUI, value: "Brave Ads Rewards!", comment: "")
  static let notificationAdsTitle = NSLocalizedString("BraveRewardsNotificationAdsTitle", bundle: .rewardsUI, value: "Brave Ads", comment: "")
  static let minimumVisitsChoices2 = NSLocalizedString("BraveRewardsMinimumVisitsChoices2", bundle: .rewardsUI, value: "10 visits", comment: "")
  static let minimumVisitsChoices1 = NSLocalizedString("BraveRewardsMinimumVisitsChoices1", bundle: .rewardsUI, value: "5 visits", comment: "")
  static let minimumVisitsChoices0 = NSLocalizedString("BraveRewardsMinimumVisitsChoices0", bundle: .rewardsUI, value: "1 visit", comment: "")
  static let addFundsVCTitle = NSLocalizedString("BraveRewardsAddFundsVCTitle", bundle: .rewardsUI, value: "Add Funds", comment: "")
  static let addFundsTitle = NSLocalizedString("BraveRewardsAddFundsTitle", bundle: .rewardsUI, value: "Send cryptocurrency from your external account to your Brave Rewards wallet.", comment: "")
  static let notificationAutoContributeTitle = NSLocalizedString("BraveRewardsNotificationAutoContributeTitle", bundle: .rewardsUI, value: "Auto-Contribute", comment: "")
  static let addFunds = NSLocalizedString("BraveRewardsAddFunds", bundle: .rewardsUI, value: "Add Funds", comment: "")
  static let withdrawFunds = NSLocalizedString("BraveRewardsWithdrawFunds", bundle: .rewardsUI, value: "Withdraw Funds", comment: "")
  static let emptyTipsText = NSLocalizedString("BraveRewardsEmptyTipsText", bundle: .rewardsUI, value: "Have you tipped your favorite content creator today?", comment: "")
  static let settings = NSLocalizedString("BraveRewardsSettings", bundle: .rewardsUI,
                                                      value: "Settings", comment: "")
  static let autoContributeMinimumVisits = NSLocalizedString("BraveRewardsAutoContributeMinimumVisits", bundle: .rewardsUI, value: "Minimum Visits", comment: "")
  static let addFundsShowQRCode = NSLocalizedString("BraveRewardsAddFundsShowQRCode", bundle: .rewardsUI, value: "Show QR Code", comment: "")
  static let rewardsOptInPrefix = NSLocalizedString("RewardsOptInPrefix", bundle: .rewardsUI, value: "Get ready to experience the next Internet.", comment: "")
  static let disclaimerLearnMore = NSLocalizedString("BraveRewardsDisclaimerLearnMore", bundle: .rewardsUI, value: "Learn More", comment: "")
  static let showAllPendingContributions = NSLocalizedString("BraveRewardsShowAllPendingContributions", bundle: .rewardsUI, value: "Show all pending contributions", comment: "")
  static let CLAIM = NSLocalizedString("CLAIM", bundle: .rewardsUI, value: "CLAIM", comment: "")
  static let autoContributeNextDate = NSLocalizedString("BraveRewardsAutoContributeNextDate", bundle: .rewardsUI, value: "Next contribution date", comment: "")
  static let monthlyTippingNextDate = NSLocalizedString("BraveRewardsMonthlyTippingNextDate", bundle: .rewardsUI, value: "Next monthly contribution date", comment: "")
  static let tippingMakeMonthly = NSLocalizedString("BraveRewardsTippingMakeMonthly", bundle: .rewardsUI, value: "Make this monthly", comment: "")
  static let ok = NSLocalizedString("OK", bundle: .rewardsUI, value: "OK", comment: "")
  static let learnMoreHowItWorks = NSLocalizedString("BraveRewardsLearnMoreHowItWorks", bundle: .rewardsUI, value: "How it works…", comment: "")
  static let grantListExpiresOn = NSLocalizedString("BraveRewardsGrantListExpiresOn", bundle: .rewardsUI, value: "Expires on %@", comment: "")
  static let autoContribute = NSLocalizedString("BraveRewardsAutoContribute", bundle: .rewardsUI, value: "Auto-Contribute", comment: "")
  static let poweredByUphold = NSLocalizedString("BraveRewardsPoweredByUphold", bundle: .rewardsUI, value: "Your Brave wallet is powered by Uphold", comment: "")
  static let addFundsBody = NSLocalizedString("BraveRewardsAddFundsBody", bundle: .rewardsUI, value: "Be sure to use the address below that matches the type of crypto you own. It will be converted automatically to BAT by Uphold and appear as an increased balance in your Brave Rewards wallet. Please allow up to one hour for your wallet balance to update.", comment: "")
  static let site = NSLocalizedString("BraveRewardsSite", bundle: .rewardsUI, value: "Site", comment: "")
  static let totalSites = NSLocalizedString("BraveRewardsTotalSites", bundle: .rewardsUI, value: "Total %ld", comment: "")
  static let emptyWalletBody = NSLocalizedString("BraveRewardsEmptyWalletBody", bundle: .rewardsUI, value: "Watch your balance grow as you view privacy preserving ads through Brave.", comment: "")
  static let braveRewards = NSLocalizedString("BraveRewards", bundle: .rewardsUI, value: "Brave Rewards™", comment: "")
  static let publisherSendTip = NSLocalizedString("BraveRewardsPublisherSendTip", bundle: .rewardsUI, value: "Send a tip…", comment: "")
  static var tippingWalletBalanceTitle: String {
    if Preferences.Rewards.isUsingBAP.value == true {
      return NSLocalizedString("BraveRewardsTippingBalanceTitle", bundle: .rewardsUI, value: "balance", comment: "")
    }
    return NSLocalizedString("BraveRewardsTippingWalletBalanceTitle", bundle: .rewardsUI, value: "wallet balance", comment: "")
  }
  static let learnMoreSubtitle = NSLocalizedString("BraveRewardsLearnMoreSubtitle", bundle: .rewardsUI, value: "Get Rewarded for Browsing!", comment: "")
  static let tippingAmountTitle = NSLocalizedString("BraveRewardsTippingAmountTitle", bundle: .rewardsUI, value: "Tip amount", comment: "")
  static let minimumLengthChoices2 = NSLocalizedString("BraveRewardsMinimumLengthChoices2", bundle: .rewardsUI, value: "1 minute", comment: "")
  static let minimumLengthChoices1 = NSLocalizedString("BraveRewardsMinimumLengthChoices1", bundle: .rewardsUI, value: "8 seconds", comment: "")
  static let minimumLengthChoices0 = NSLocalizedString("BraveRewardsMinimumLengthChoices0", bundle: .rewardsUI, value: "5 seconds", comment: "")
  static let settingsAdsTitle = NSLocalizedString("BraveRewardsSettingsAdsTitle", bundle: .rewardsUI, value: "Ads", comment: "")
  static let notificationRecurringTipTitle = NSLocalizedString("BraveRewardsNotificationRecurringTipTitle", bundle: .rewardsUI, value: "Recurring tips", comment: "")
  static let emptyWalletTitle = NSLocalizedString("BraveRewardsEmptyWalletTitle", bundle: .rewardsUI, value: "Your wallet is ready for action.", comment: "")
  static let recurringTipTitle = NSLocalizedString("BraveRewardsRecurringTipTitle", bundle: .rewardsUI, value: "Recurring tip", comment: "")
  static let unverifiedPublisherDisclaimer = NSLocalizedString("BraveRewardsUnverifiedPublisherDisclaimer", bundle: .rewardsUI, value: "This creator has not yet signed up to receive contributions from Brave users. Any tips you send will remain in your wallet until they verify.", comment: "")
  static let connectedPublisherDisclaimer = NSLocalizedString("BraveRewardsConnectedPublisherDisclaimer", bundle: .rewardsUI, value: "This Brave Verified Creator has not yet configured their account to receive contributions from Brave Users. Any tips you send will remain in your wallet until they complete this process.", comment: "")
  static let settingsGrantClaimButtonTitle = NSLocalizedString("BraveRewardsSettingsGrantClaimButtonTitle", bundle: .rewardsUI, value: "Claim", comment: "")
  static let autoContributeMinimumLength = NSLocalizedString("BraveRewardsAutoContributeMinimumLength", bundle: .rewardsUI, value: "Minimum Page Time", comment: "")
  static let autoContributeMinimumVisitsMessage = NSLocalizedString("BraveRewardsAutoContributeMinimumVisitsMessage", bundle: .rewardsUI, value: "Minimum visits for publisher relevancy", comment: "")
  static let autoContributeMinimumLengthMessage = NSLocalizedString("BraveRewardsAutoContributeMinimumLengthMessage", bundle: .rewardsUI, value: "Minimum page time before logging a visit", comment: "")
  static var notificationTokenGrantTitle: String {
    if Preferences.Rewards.isUsingBAP.value == true {
        return NSLocalizedString("BraveRewardsNotificationPointGrantTitle", bundle: .rewardsUI, value: "Point Grants", comment: "")
    }
    return NSLocalizedString("BraveRewardsNotificationTokenGrantTitle", bundle: .rewardsUI, value: "Token Grants", comment: "")
  }
  static let learnMoreBraveAdsBody = NSLocalizedString("BraveRewardsLearnMoreBraveAdsBody", bundle: .rewardsUI, value: "Get paid to view relevant ads that respect your privacy.", comment: "")
  static let autoContributeToUnverifiedSites = NSLocalizedString("BraveRewardsAutoContributeToUnverifiedSites", bundle: .rewardsUI, value: "Allow contribution to non-verified sites", comment: "")
  static let learnMoreWhyTitle = NSLocalizedString("BraveRewardsLearnMoreWhyTitle", bundle: .rewardsUI, value: "Why Brave Rewards?", comment: "")
  static let attention = NSLocalizedString("BraveRewardsAttention", bundle: .rewardsUI, value: "Attention", comment: "")
  static let exclude = NSLocalizedString("BraveRewardsExclude", bundle: .rewardsUI, value: "Exclude", comment: "")
  static let autoContributeSupportedSites = NSLocalizedString("BraveRewardsAutoContributeSupportedSites", bundle: .rewardsUI, value: "Supported sites", comment: "")
  static let learnMoreTurnOnRewardsTitle = NSLocalizedString("BraveRewardsLearnMoreTurnOnRewardsTitle", bundle: .rewardsUI, value: "Activate Rewards", comment: "")
  static let settingsDisabledTitle1 = NSLocalizedString("BraveRewardsSettingsDisabledTitle1", bundle: .rewardsUI, value: "Why Brave Rewards?", comment: "")
  static let settingsDisabledTitle2 = NSLocalizedString("BraveRewardsSettingsDisabledTitle2", bundle: .rewardsUI, value: "Today, Brave welcomes you to the new internet.", comment: "")
  static let summaryTitle = NSLocalizedString("BraveRewardsSummaryTitle", bundle: .rewardsUI, value: "Rewards Summary", comment: "")
  static var settingsGrantText: String {
    if Preferences.Rewards.isUsingBAP.value == true {
      return NSLocalizedString("BraveRewardsSettingsGrantTextJapan", bundle: .rewardsUI, value: "A free point grant is available.", comment: "")
    }
    return NSLocalizedString("BraveRewardsSettingsGrantText", bundle: .rewardsUI, value: "A free token grant is available.", comment: "")
  }
  static let settingsAdsGrantText = NSLocalizedString("BraveRewardsSettingsAdsGrantText", bundle: .rewardsUI, value: "earned from ads", comment: "Example: <10 BAT> earned from ads")
  static let settingsAdsGrantAmountText = NSLocalizedString("BraveRewardsSettingsAdsGrantAmountText", bundle: .rewardsUI, value: "Your Ads earnings, %@ are available.", comment: "")
  static let grants = NSLocalizedString("BraveRewardsGrants", bundle: .rewardsUI, value: "Grants", comment: "")
  static let learnMoreTipsBody = NSLocalizedString("BraveRewardsLearnMoreTipsBody", bundle: .rewardsUI, value: "Support your favorite sites just by browsing – or tip a site any time you like.", comment: "")
  static let walletDetailsTitle = NSLocalizedString("BraveRewardsWalletDetailsTitle", bundle: .rewardsUI, value: "Wallet Details", comment: "")
  static var grantsClaimedSubtitle: String {
    if Preferences.Rewards.isUsingBAP.value == true {
      return NSLocalizedString("BraveRewardsGrantsClaimedSubtitleJapan", bundle: .rewardsUI, value: "Your point grant is on its way.", comment: "")
    }
    return NSLocalizedString("BraveRewardsGrantsClaimedSubtitle", bundle: .rewardsUI, value: "Your token grant is on its way.", comment: "")
  }
  static let adsGrantsClaimedSubtitle = NSLocalizedString("BraveRewardsAdsGrantsClaimedSubtitle", bundle: .rewardsUI, value: "Your rewards grant from Brave Ads is on its way.", comment: "")
  static let autoContributeRestoreExcludedSites = NSLocalizedString("BraveRewardsAutoContributeRestoreExcludedSites", bundle: .rewardsUI, value: "Restore %ld excluded sites", comment: "")
  static let settingsViewDetails = NSLocalizedString("BraveRewardsSettingsViewDetails", bundle: .rewardsUI, value: "View Details", comment: "")
  static let autoContributeMonthlyPaymentTitle = NSLocalizedString("BraveRewardsAutoContributeMonthlyPaymentTitle", bundle: .rewardsUI, value: "Monthly Payment", comment: "")
  static let autoContributeMonthlyPayment = NSLocalizedString("BraveRewardsAutoContributeMonthlyPayment", bundle: .rewardsUI, value: "Monthly payment", comment: "")
  static var tokens: String {
    if Preferences.Rewards.isUsingBAP.value == true {
      return NSLocalizedString("BraveRewardsPoints", bundle: .rewardsUI, value: "Points", comment: "")
    }
    return NSLocalizedString("BraveRewardsTokens", bundle: .rewardsUI, value: "Tokens", comment: "")
  }
  static let learnMoreTurnOnRewardsBody = NSLocalizedString("BraveRewardsLearnMoreTurnOnRewardsBody", bundle: .rewardsUI, value: "This enables both ads and automatic contributions. You can turn them on or off separately at any time.", comment: "")
  static var tippingNotEnoughTokens: String {
    if Preferences.Rewards.isUsingBAP.value == true {
      return NSLocalizedString("BraveRewardsTippingNotEnoughPoints", bundle: .rewardsUI, value: "Not enough points.", comment: "")
    }
    return NSLocalizedString("BraveRewardsTippingNotEnoughTokens", bundle: .rewardsUI, value: "Not enough tokens.", comment: "")
  }
  static let tippingOverviewTitle = NSLocalizedString("BraveRewardsTippingOverviewTitle", bundle: .rewardsUI, value: "Welcome!", comment: "")
  static let learnMoreTipsTitle = NSLocalizedString("BraveRewardsLearnMoreTipsTitle", bundle: .rewardsUI, value: "Auto-Contribute", comment: "")
  static let tippingConfirmation = NSLocalizedString("BraveRewardsTippingConfirmation", bundle: .rewardsUI, value: "Thank you", comment: "")
  static let tippingMonthlyTitle = NSLocalizedString("BraveRewardsTippingMonthlyTitle", bundle: .rewardsUI, value: "You are automatically sending a tip to:", comment: "")
  static let tippingOneTimeTitle = NSLocalizedString("BraveRewardsTippingOneTimeTitle", bundle: .rewardsUI, value: "You've just sent a tip to:", comment: "")
  static let tippingRecurring = NSLocalizedString("BraveRewardsTippingRecurring", bundle: .rewardsUI, value: "Monthly", comment: "")
  static let tippingRecurringDetails = NSLocalizedString("BraveRewardsTippingRecurringDetails", bundle: .rewardsUI, value: "Your first monthly tip will be sent on:", comment: "")
  static let disabledBody = NSLocalizedString("BraveRewardsDisabledBody", bundle: .rewardsUI, value: "Earn by viewing privacy-respecting ads, and pay it forward to support content creators you love.", comment: "")
  static let learnMoreCreateWallet2 = NSLocalizedString("BraveRewardsLearnMoreCreateWallet2", bundle: .rewardsUI, value: "Yes I'm Ready!", comment: "")
  static let disabledTitle = NSLocalizedString("BraveRewardsDisabledTitle", bundle: .rewardsUI, value: "Welcome Back!", comment: "")
  static let learnMoreCreateWallet1 = NSLocalizedString("BraveRewardsLearnMoreCreateWallet1", bundle: .rewardsUI, value: "Yes, I'm In!", comment: "")
  static let learnMoreBraveAdsTitle = NSLocalizedString("BraveRewardsLearnMoreBraveAdsTitle", bundle: .rewardsUI, value: "Ads", comment: "")
  static let tippingUnverifiedDisclaimer = NSLocalizedString("BraveRewardsTippingUnverifiedDisclaimer", bundle: .rewardsUI, value: "NOTE: This creator has not yet signed up to receive contributions from Brave users. Your browser will keep trying to contribute until they verify, or until 90 days have passed.", comment: "")
  static let tippingNotConnectedDisclaimer = NSLocalizedString("BraveRewardsTippingNotConnectedDisclaimer", bundle: .rewardsUI, value: "NOTE: This Brave Verified Creator has not yet signed up to receive contributions from Brave users. Your browser will keep trying to contribute until they verify, or until 90 days have passed.", comment: "")
  static let settingsTipsBody = NSLocalizedString("BraveRewardsSettingsTipsBody", bundle: .rewardsUI, value: "Tip content creators directly as you browse by using the Rewards Panel.", comment: "")
  static let settingsMonthlyTipsBody = NSLocalizedString("BraveRewardsSettingsMonthlyTipsBody", bundle: .rewardsUI, value: "Set up recurring monthly contributions so you can support sites continuously.", comment: "")
  static let cancel = NSLocalizedString("Cancel", bundle: .rewardsUI, value: "Cancel", comment: "")
  static let disabledSubtitle = NSLocalizedString("BraveRewardsDisabledSubtitle", bundle: .rewardsUI, value: "Get Rewarded for Browsing!", comment: "")
  static let emptyAutoContribution = NSLocalizedString("BraveRewardsEmptyAutoContribution", bundle: .rewardsUI, value: "Sites will appear as you browse", comment: "")
  static let learnMoreBody = NSLocalizedString("BraveRewardsLearnMoreBody", bundle: .rewardsUI, value: "Your attention is valuable. Earn by viewing privacy-respecting ads, and pay it forward to support content creators you love.", comment: "")
  static let rewardsOptInJoinTitle = NSLocalizedString("RewardsOptInJoinTitle", bundle: .rewardsUI, value: "Join Rewards", comment: "")
  static let disclaimerInformation = NSLocalizedString("DisclaimerInformation", bundle: .rewardsUI, value: "By clicking 'Join Rewards', you indicate that you have read and agree to the Terms of Service and Privacy Policy.", comment: "")
  static let welcomeDisclaimerInformation = NSLocalizedString("WelcomeDisclaimerInformation", bundle: .rewardsUI, value: "By clicking 'Yes, I'm in!', you indicate that you have read and agree to the Terms of Service and Privacy Policy.", comment: "")
  static let termsOfServiceURL = NSLocalizedString("TermsOfServiceURL", bundle: .rewardsUI, value: "Terms of Service", comment: "")
  static let privacyPolicyURL = NSLocalizedString("PrivacyPolicyURL", bundle: .rewardsUI, value: "Privacy Policy", comment: "")
  static let addFundsTokenWalletAddress = NSLocalizedString("BraveRewardsAddFundsTokenWalletAddress", bundle: .rewardsUI, value: "Wallet Address", comment: "")
  static let autoContributeToVideos = NSLocalizedString("BraveRewardsAutoContributeToVideos", bundle: .rewardsUI, value: "Allow contribution to videos", comment: "")
  static let settingsDisabledBody2 = NSLocalizedString("BraveRewardsSettingsDisabledBody2", bundle: .rewardsUI, value: "One where your time is valued, your personal data is kept private, and you actually get paid for your attention.", comment: "")
  static let contributingToUnverifiedSites = NSLocalizedString("BraveRewardsContributingToUnverifiedSites", bundle: .rewardsUI, value: "You've designated %@ for creators who haven't yet signed up to recieve contributions. Your browser will keep trying to contribute until they verify, or until 90 days have passed.", comment: "")
  static let settingsDisabledBody1 = NSLocalizedString("BraveRewardsSettingsDisabledBody1", bundle: .rewardsUI, value: "With your old browser, you paid to browse the web by viewing ads with your valuable attention. You spent your valuable time downloading invasive ad technology that transmitted your valuable private data to advertisers — without your consent.", comment: "")
  static let panelTitle = NSLocalizedString("BraveRewardsPanelTitle", bundle: .rewardsUI, value: "Rewards", comment: "")
  static let creatingWallet = NSLocalizedString("BraveRewardsCreatingWallet", bundle: .rewardsUI, value: "Creating wallet", comment: "")
  static var rewardsOptInDescription: String {
    if Preferences.Rewards.isUsingBAP.value == true {
      return NSLocalizedString("RewardsOptInDescriptionJapan", bundle: .rewardsUI, value: "You can now earn points for watching privacy-respecting ads.", comment: "")
    }
    return NSLocalizedString("RewardsOptInDescription", bundle: .rewardsUI, value: "You can now earn tokens for watching privacy-respecting ads.", comment: "")
  }
  static var walletHeaderTitle: String {
    if Preferences.Rewards.isUsingBAP.value == true {
      return NSLocalizedString("BraveRewardsWalletHeaderTitleJapan", bundle: .rewardsUI, value: "Your balance", comment: "")
    }
    return NSLocalizedString("BraveRewardsWalletHeaderTitle", bundle: .rewardsUI, value: "Your wallet", comment: "")
  }
  static let tipsTotalThisMonth = NSLocalizedString("BraveRewardsTipsTotalThisMonth", bundle: .rewardsUI, value: "Total tips this month", comment: "")
  static let monthlyContributionsTotalThisMonth = NSLocalizedString("BraveRewardsMonthlyContributionsTotalThisMonth", bundle: .rewardsUI, value: "Total contributions this month", comment: "")
  static let tippingTitle = NSLocalizedString("BraveRewardsTippingTitle", bundle: .rewardsUI, value: "Send a tip", comment: "")
  static let grantsClaimedExpirationDateTitle = NSLocalizedString("BraveRewardsGrantsClaimedExpirationDateTitle", bundle: .rewardsUI, value: "Grant Expiration Date", comment: "")
  static let settingsAutoContributeBody = NSLocalizedString("BraveRewardsSettingsAutoContributeBody", bundle: .rewardsUI, value: "An automatic way to support publishers and content creators. Set a monthly payment and browse normally. The Brave Verified sites you visit will receive your contributions automatically, based on your attention as measured by Brave.", comment: "")
  static var grantsClaimedAmountTitle: String {
    if Preferences.Rewards.isUsingBAP.value == true {
      return NSLocalizedString("BraveRewardsGrantsClaimedAmountTitleJapan", bundle: .rewardsUI, value: "Free Points Grant", comment: "")
    }
    return NSLocalizedString("BraveRewardsGrantsClaimedAmountTitle", bundle: .rewardsUI, value: "Free Token Grant", comment: "")
  }
  static var adsGrantsClaimedAmountTitle: String {
    if Preferences.Rewards.isUsingBAP.value == true {
      return NSLocalizedString("BraveRewardsAdsGrantsClaimedAmountTitleJapan", bundle: .rewardsUI, value: "Your Brave Ads Point Grant", comment: "")
    }
    return NSLocalizedString("BraveRewardsAdsGrantsClaimedAmountTitle", bundle: .rewardsUI, value: "Your Brave Ads Token Grant", comment: "")
  }
  static let settingsAdsComingSoonText = NSLocalizedString("BraveRewardsSettingsAdsComingSoonText", bundle: .rewardsUI, value: "Coming soon.", comment: "")
  static let settingsTipsTitle = NSLocalizedString("BraveRewardsSettingsTipsTitle", bundle: .rewardsUI, value: "Tips", comment: "")
  static let settingsMonthlyTipsTitle = NSLocalizedString("BraveRewardsSettingsMonthlyTipsTitle", bundle: .rewardsUI, value: "Monthly Contributions", comment: "")
  static let addFundsDisclaimer = NSLocalizedString("BraveRewardsAddFundsDisclaimer", bundle: .rewardsUI, value: "Reminder: The Brave Wallet is unidirectional and BAT flows to publisher sites. For more information about Brave Rewards, please visit the FAQ.", comment: "")
  static let learnMoreReady = NSLocalizedString("BraveRewardsLearnMoreReady", bundle: .rewardsUI, value: "Ready to get started?", comment: "")
  static let remove = NSLocalizedString("BraveRewardsRemove", bundle: .rewardsUI, value: "Remove", comment: "")
  static let learnMoreWhyBody = NSLocalizedString("BraveRewardsLearnMoreWhyBody", bundle: .rewardsUI, value: "With your old browser, you paid to browse the web by viewing ads with your valuable attention. You spent your valuable time downloading invasive ad technology that transmitted your valuable private data to advertisers — without your consent.\n\nToday, Brave welcomes you to the new Internet. One where your time is valued, your personal data is kept private, and you actually get paid for your attention.", comment: "")
  static let disabledEnableButton = NSLocalizedString("BraveRewardsDisabledEnableButton", bundle: .rewardsUI, value: "Enable Brave Rewards", comment: "")
  static let tippingOverviewBody = NSLocalizedString("BraveRewardsTippingOverviewBody", bundle: .rewardsUI, value: "You can support this content creator by sending a tip. It’s a way of thanking them for making great content. Verified creators get paid for their tips during the first week of each calendar month.\n\nIf you like, you can schedule monthly tips to support this creator on a continuous basis.", comment: "")
  static let tippingSendTip = NSLocalizedString("BraveRewardsTippingSendTip", bundle: .rewardsUI, value: "Send my Tip", comment: "")
  static let tippingSendMonthlyTip = NSLocalizedString("BraveRewardsTippingSendMonthlyTip", bundle: .rewardsUI, value: "Set monthly tip", comment: "")
  static let noActivitiesYet = NSLocalizedString("BraveRewardsNoActivitiesYet", bundle: .rewardsUI, value: "No activities yet…", comment: "")
  static let adsMaxPerHour = NSLocalizedString("BraveRewardsAdsMaxPerDay", bundle: .rewardsUI, value: "Maximum number of ads displayed", comment: "")
  static let numberOfAdsPerHourOptionsTitle = NSLocalizedString("BraveRewardsNumberOfAdsPerHourOptionsTitle", bundle: .rewardsUI, value: "Ads per hour", comment: "")
  static let adsEstimatedEarnings = NSLocalizedString("BraveRewardsAdsEstimatedEarnings", bundle: .rewardsUI, value: "Estimated pending rewards", comment: "")
  static let nextPaymentDate = NSLocalizedString("BraveRewardsPaymentDate", bundle: .rewardsUI, value: "Next payment date", comment: "")
  static let adNotificationsReceived = NSLocalizedString("BraveRewardsAdNotificationsReceived", bundle: .rewardsUI, value: "Ad notifications received this month", comment: "")
  static let oneAdPerHour = NSLocalizedString("BraveRewardsOneAdPerHour", bundle: .rewardsUI, value: "1 ad per hour", comment: "")
  static let twoAdsPerHour = NSLocalizedString("BraveRewardsTwoAdsPerHour", bundle: .rewardsUI, value: "2 ads per hour", comment: "")
  static let threeAdsPerHour = NSLocalizedString("BraveRewardsThreeAdsPerHour", bundle: .rewardsUI, value: "3 ads per hour", comment: "")
  static let fourAdsPerHour = NSLocalizedString("BraveRewardsFourAdsPerHour", bundle: .rewardsUI, value: "4 ads per hour", comment: "")
  static let fiveAdsPerHour = NSLocalizedString("BraveRewardsFiveAdsPerHour", bundle: .rewardsUI, value: "5 ads per hour", comment: "")
  static let adsPayoutDateFormat = NSLocalizedString("BraveRewardsAdsPayoutDateFormat", bundle: .rewardsUI, value: "MMM d", comment: "")
  static let autoContributeDateFormat = NSLocalizedString("BraveRewardsAutoContributeDateFormat", bundle: .rewardsUI, value: "MMM d", comment: "")
  static var totalGrantsClaimed: String {
    if Preferences.Rewards.isUsingBAP.value == true {
      return NSLocalizedString("BraveRewardsTotalGrantsClaimedJapan", bundle: .rewardsUI, value: "Point Grants Claimed", comment: "")
    }
    return NSLocalizedString("BraveRewardsTotalGrantsClaimed", bundle: .rewardsUI, value: "Token Grants Claimed", comment: "")
  }
  static let earningFromAds = NSLocalizedString("BraveRewardsEarningFromAds", bundle: .rewardsUI, value: "Earnings from Ads", comment: "")
  static let oneTimeTips = NSLocalizedString("BraveRewardsOneTimeTips", bundle: .rewardsUI, value: "One-time Tips", comment: "")
  static let monthlyTips = NSLocalizedString("BraveRewardsMonthlyTips", bundle: .rewardsUI, value: "Monthly Tips", comment: "")
  static let adsUnsupportedRegion = NSLocalizedString("BraveRewardsAdsUnsupportedRegion", bundle: .rewardsUI, value: "Sorry! Ads are not yet available in your region.", comment: "")
  static let adsUnsupportedDevice = NSLocalizedString("BraveRewardsAdsUnsupportedDvice", bundle: .rewardsUI, value: "Brave Rewards and Ads are not available on your device at this time.", comment: "")
  static let autoContributeSwitchLabel = NSLocalizedString("AutoContributeSwitchLabel", bundle: .rewardsUI, value: "Include in Auto-Contribute", comment: "Label for auto-contribute toggle.")
  static let tipSiteMonthly = NSLocalizedString("TipSiteMonthly", bundle: .rewardsUI, value: "Tip this site monthly", comment: "")
  static let oneTimeText = NSLocalizedString("OneTimeText", bundle: .rewardsUI, value: "One time ", comment: "Text describing the type of contribution")
  static let recurringText = NSLocalizedString("RecurringText", bundle: .rewardsUI, value: "Recurring", comment: "Text describing the type of contribution")
  static let onProviderText = NSLocalizedString("OnProviderText", bundle: .rewardsUI, value: "on %@", comment: "This is a suffix statement. example: SomeChannel on Twitter")
  static let notificationInsufficientFundsTitle = NSLocalizedString("NotificationInsufficientFundsTitle", bundle: .rewardsUI, value: "Insufficient Funds", comment: "Title for insufficient funds notification")
  static let turnOnAds = NSLocalizedString("TurnOnAds", bundle: .rewardsUI, value: "Turn on Ads", comment: "Prompt to turn on Ads via notification")
  static let notificationPendingContributionTitle = NSLocalizedString("NotificationPendingContributionTitle", bundle: .rewardsUI, value: "Pending Contribution", comment: "Notification title for pending contribution type")
  static let notificationTipsProcessedBody = NSLocalizedString("NotificationTipsProcessedBody", bundle: .rewardsUI, value: "Your monthly tips have been processed!", comment: "Message for monthly tips processed notification")
  static let notificationVerifiedPublisherBody = NSLocalizedString("NotificationVerifiedPublisherBody", bundle: .rewardsUI, value: "Creator %@ recently verified", comment: "Notification text that tells user which publisher just verified")
  static let notificationAutoContributeNotEnoughFundsBody = NSLocalizedString("NotificationAutoContributeNotEnoughFundsBody", bundle: .rewardsUI, value: "Your scheduled monthly payment for Auto-Contribute and monthly tips could not be completed due to insufficient funds. We’ll try again in 30 days.", comment: "We show this string in the notification when you don't have enough funds for contribution")
  static let notificationContributeTipError = NSLocalizedString("NotificationContributeTipError", bundle: .rewardsUI, value: "Unable to send your tip. Please try again later.", comment: "We show this string in notification when tip fails")
  static let notificationContributeError = NSLocalizedString("NotificationContributeError", bundle: .rewardsUI, value: "There was a problem processing your contribution.", comment: "We show this string in notification when contribution fails")
  static let notificationContributeSuccess = NSLocalizedString("NotificationContributeSuccess", bundle: .rewardsUI, value: "You've contributed %@", comment: "We show this string in the notification when contribution is successful")
  static let notificationBraveAdsLaunchMsg = NSLocalizedString("NotificationBraveAdsLaunchMsg", bundle: .rewardsUI, value: "Now you can earn by viewing ads.", comment: "Message for ads launch notification")
  static let notificationContributeNotificationError = NSLocalizedString("NotificationContributeNotificationError", bundle: .rewardsUI, value: "There was a problem processing your contribution.", comment: "We show this string in notification when contribution fails")
  static let notificationPendingNotEnoughFunds = NSLocalizedString("NotificationPendingNotEnoughFunds", bundle: .rewardsUI, value: "You have pending tips due to insufficient funds", comment: "Notification text that tells user his wallet is under funded for pending contribution to complete")
  static let notificationInsufficientFunds = NSLocalizedString("NotificationinsufficientFunds", bundle: .rewardsUI, value: "Your Brave Rewards account is waiting for a deposit.", comment: "Description for new insufficient funds notification")
  static let notificationEarningsClaimDefault = NSLocalizedString("NotificationEarningsClaimDefault", bundle: .rewardsUI, value: "Your rewards from Ads are here!", comment: "Panel notification text for Ads grant")
  static let notificationGrantNotification = NSLocalizedString("NotificationGrantNotification", bundle: .rewardsUI, value: "You have a grant waiting for you.", comment: "Description for new grant notification")
  static let notificationErrorTitle = NSLocalizedString("NotificationErrorTitle", bundle: .rewardsUI, value: "Uh oh!", comment: "Title for an error notification")
  static let noNetworkTitle = NSLocalizedString("NoNetworkTitle", bundle: .rewardsUI, value: "Uh oh!", comment: "Title for a no network notification")
  static let noNetworkBody = NSLocalizedString("NoNetworkBody", bundle: .rewardsUI, value: "The Brave Rewards server is not responding. We will fix this as soon as possible.", comment: "Body for a no network notification")
  static let myFirstAdTitle = NSLocalizedString("MyFirstAdTitle", bundle: .rewardsUI, value: "This is your first Brave ad", comment: "")
  static let myFirstAdBody = NSLocalizedString("MyFirstAdBody", bundle: .rewardsUI, value: "Tap here to learn more.", comment: "")
  static let genericErrorTitle = NSLocalizedString("WalletCreationErrorTitle", bundle: .rewardsUI, value: "Error", comment: "")
  static let genericErrorBody = NSLocalizedString("WalletCreationErrorBody", bundle: .rewardsUI, value: "Oops! Something went wrong. Please try again.", comment: "")
  static let BATPointsDisclaimer = NSLocalizedString("BATPointsDisclaimer", bundle: .rewardsUI, value: "BAT Points can be used to contribute to your favorite content creators. BAT Points cannot be exchanged for BAT.", comment: "Disclaimer about BAT Points for JP users")
  static let BATPointsDisclaimerBoldedWords = NSLocalizedString("BATPointsBoldedWords", bundle: .rewardsUI, value: "BAT Points", comment: "Words that should be bolded in the BAT Points disclaimer")
  
  static let disabledAutoContributeMessage = NSLocalizedString("DisabledAutoContributeMessage", bundle: .rewardsUI, value: "Reward creators for the content you love. Your monthly payment gets distributed across the sites you visit.", comment: "Message that is displayed when user disables auto-contribute")
  static let disabledAdsMessage = NSLocalizedString("DisabledAdsMessage", bundle: .rewardsUI, value: "Earnings are paid every month. Set your desired frequency to increase or decrease earnings.", comment: "Message that is displayed when user disables ads")
  static let userWalletOnboardingTitle =  NSLocalizedString("UserWalletTitle", bundle: .rewardsUI, value: "Verifying your wallet is optional", comment: "The title of the user wallet onboarding screen")
  static let userWalletOnboardingBenefitsTitle =  NSLocalizedString("UserWalletOnboardingBenefitsTitle", bundle: .rewardsUI, value: "But if you verify, you can…", comment: "The title above the list of benefits after verifying")
  static let userWalletOnboardingBenefitsOne =  NSLocalizedString("UserWalletOnboardingBenefitsOne", bundle: .rewardsUI, value: "Withdraw BAT that you earn from viewing privacy-respecting ads", comment: "The first benefit of verifying with Uphold")
  static let userWalletOnboardingBenefitsTwo =  NSLocalizedString("UserWalletOnboardingBenefitsTwo", bundle: .rewardsUI, value: "Purchase additional BAT with credit cards and other sources", comment: "The second benefit of verifying with Uphold")
  static let userWalletOnboardingBenefitsThree =  NSLocalizedString("UserWalletOnboardingBenefitsThree", bundle: .rewardsUI, value: "Withdraw BAT that you may have previously added to your Brave Rewards wallet", comment: "The third benefit of verifying with Uphold")
  static let userWalletOnboardingVerifyButtonTitle =  NSLocalizedString("UserWalletOnboardingVerifyButtonTitle", bundle: .rewardsUI, value: "Verify Wallet", comment: "The button that triggers the Uphold auth process")
  static let userWalletOnboardingPoweredByUphold =  NSLocalizedString("UserWalletOnboardingPoweredByUphold", bundle: .rewardsUI, value: "Our wallet service is provided by Uphold", comment: "")
  static let userWalletOnboardingPoweredByUpholdBoldedWord =  NSLocalizedString("UserWalletOnboardingPoweredByUpholdBoldedWord", bundle: .rewardsUI, value: "Uphold", comment: "The word that will be bolded in UserWalletOnboardingPoweredByUphold")
  static let userWalletOnboardingUpholdDisclosure =  NSLocalizedString("UserWalletOnboardingUpholdDisclosure", bundle: .rewardsUI, value: "Uphold may require you to verify your identity based on services requested.\n\nBrave Software Inc. does not process, store, or access any of the personal information that you provide to Uphold when you establish an account with them.", comment: "The disclosure about Uphold at the bottom of the screen")
  static let userWalletUnverifiedButtonTitle =  NSLocalizedString("UserWalletUnverifiedButtonTitle", bundle: .rewardsUI, value: "Verify Wallet", comment: "")
  static let userWalletDisconnectedButtonTitle =  NSLocalizedString("UserWalletDisconnectedButtonTitle", bundle: .rewardsUI, value: "Disconnected", comment: "")
  static let userWalletVerifiedButtonTitle =  NSLocalizedString("UserWalletVerifiedButtonTitle", bundle: .rewardsUI, value: "Wallet Verified", comment: "")
  static let userWalletDetailsTitle = NSLocalizedString("UserWalletDetailsTitle", bundle: .rewardsUI, value: "User Wallet", comment: "The title in the navigation bar shown when you click on a connected or verified user wallet button")
  static let userWalletDetailsVerified = NSLocalizedString("UserWalletDetailsVerified", bundle: .rewardsUI, value: "Verified", comment: "")
  static let userWalletDetailsAccountButtonTitle = NSLocalizedString("UserWalletDetailsAccountButtonTitle", bundle: .rewardsUI, value: "Go to my Uphold account", comment: "")
  static let userWalletDetailsCompleteVerificationButtonTitle = NSLocalizedString("UserWalletDetailsCompleteVerificationButtonTitle", bundle: .rewardsUI, value: "Complete wallet verification", comment: "")
  static let userWalletDetailsDisconnectButtonTitle = NSLocalizedString("UserWalletDetailsDisconnectButtonTitle", bundle: .rewardsUI, value: "Disconnect from Brave Rewards", comment: "")
  static let userWalletNotificationWalletDisconnectedTitle = NSLocalizedString("UserWalletNotificationWalletDisconnectedTitle", bundle: .rewardsUI, value: "Uh oh. Your wallet is unreachable.", comment: "The message you receive when your user wallet is unreachable/disconnected")
  static let userWalletNotificationWalletDisconnectedBody = NSLocalizedString("UserWalletNotificationWalletDisconnectedBody", bundle: .rewardsUI, value: "No worries. This can happen for a variety of security reasons. Reconnecting your wallet will solve this issue.", comment: "The message you receive when your user wallet is unreachable/disconnected")
  static let userWalletNotificationNowVerifiedTitle = NSLocalizedString("UserWalletNotificationNowVerifiedTitle", bundle: .rewardsUI, value: "Your wallet is verified!", comment: "The message you receive when your user wallet has been verified")
  static let userWalletNotificationNowVerifiedBody = NSLocalizedString("UserWalletNotificationNowVerifiedBody", bundle: .rewardsUI, value: "Congrats! Your %@ wallet was successfully verified and ready to add and withdraw funds.", comment: "The message you receive when your user wallet has been verified (%@ = The user wallet name, such as \"Uphold\")")
  static let exclusionListTitle = NSLocalizedString("ExclusionListTitle", bundle: .rewardsUI, value: "Excluded sites", comment: "The title of the screen that shows a list of excluded sites for Auto-Contribute")
  static let restoreAllSitesToolbarButtonTitle = NSLocalizedString("ExclusionListRestoreAllButton", bundle: .rewardsUI, value: "Restore All Sites", comment: "The button that restores all excluded publishers while on the Auto-Contribute excluson list")
  static let restore = NSLocalizedString("ExclusionListRestore", bundle: .rewardsUI, value: "Restore", comment: "The swipe-to-delete title when restoring a single item in the Auto-Contribute exclusion list")
  static let emptyExclusionList = NSLocalizedString("EmptyExclusionList", bundle: .rewardsUI, value: "No publishers excluded", comment: "The copy the user sees when they are viewing an empty Auto-Contribute exclusion list.")
  static let pendingContributionsTitle = NSLocalizedString("PendingContributionsTitle", bundle: .rewardsUI, value: "Pending contributions", comment: "The title of the screen that shows a list of the users pending contributions")
  static let removeAllSitesToolbarButtonTitle = NSLocalizedString("PendingContributionListRemoveAllButton", bundle: .rewardsUI, value: "Remove All Sites", comment: "The button that removes all pending contributions while on the Pending Contributions list")
  static let removeSelectedToolbarButtonTitle = NSLocalizedString("PendingContributionListRemoveSelected", bundle: .rewardsUI, value: "Remove Selected", comment: "The button that removes the selected pending contributions while on the Pending Contributions list")
  static let removePendingContribution = NSLocalizedString("PendingContributionDetailRemove", bundle: .rewardsUI, value: "Remove Pending Contribution", comment: "The button that removes a specific pending contributions while on viewing said contributions details")
  static let pendingContributionType = NSLocalizedString("PendingContributionDetailType", bundle: .rewardsUI, value: "Type", comment: "")
  static let pendingContributionAmount = NSLocalizedString("PendingContributionDetailAmount", bundle: .rewardsUI, value: "Amount", comment: "")
  static let pendingContributionPendingUntil = NSLocalizedString("PendingContributionDetailPendingUntil", bundle: .rewardsUI, value: "Pending Until", comment: "")
  static let ledgerInitializationFailedTitle = NSLocalizedString("LedgerInitializationFailedTitle", bundle: .rewardsUI, value: "Sorry, something went wrong…", comment: "")
  static let ledgerDatabaseMigrationFailedBody = NSLocalizedString("LedgerDatabaseMigrationFailedBody", bundle: .rewardsUI, value: "There was a problem loading your Rewards information. Your rewards activity details will be restarted.\n\nPlease contact support@brave.com for any issues or concerns", comment: "")
}

