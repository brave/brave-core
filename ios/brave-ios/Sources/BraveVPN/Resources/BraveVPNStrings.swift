// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
@_exported import Strings

extension Strings {
  public struct VPN {
    public static let vpnName = NSLocalizedString(
      "vpn.buyVPNTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Brave Firewall + VPN",
      comment: "Title for screen to buy the VPN."
    )

    public static let poweredBy = NSLocalizedString(
      "vpn.poweredBy",
      tableName: "BraveShared",
      bundle: .module,
      value: "Powered by",
      comment: "It is used in context: 'Powered by BRAND_NAME'"
    )

    public static let updateActionCellTitle = NSLocalizedString(
      "vpn.updateActionCellTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Update",
      comment: "Cell title indicates update payment method"
    )

    public static let settingsVPNEnabled = NSLocalizedString(
      "vpn.settingsVPNEnabled",
      tableName: "BraveShared",
      bundle: .module,
      value: "Enabled",
      comment: "Whether the VPN is enabled or not"
    )

    public static let errorCantGetPricesTitle = NSLocalizedString(
      "vpn.errorCantGetPricesTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "App Store Error",
      comment: "Title for an alert when the VPN can't get prices from the App Store"
    )

    public static let errorCantGetPricesBody = NSLocalizedString(
      "vpn.errorCantGetPricesBody",
      tableName: "BraveShared",
      bundle: .module,
      value: "Could not connect to the App Store, please try again in few minutes.",
      comment: "Message for an alert when the VPN can't get prices from the App Store"
    )

    public static let settingsVPNDisabled = NSLocalizedString(
      "vpn.settingsVPNDisabled",
      tableName: "BraveShared",
      bundle: .module,
      value: "Disabled",
      comment: "Whether the VPN is enabled or not"
    )

    public static let popupCheckboxBlockAds = NSLocalizedString(
      "vpn.popupCheckboxBlockAds",
      tableName: "BraveShared",
      bundle: .module,
      value: "Blocks unwanted network connections",
      comment: "Text for a checkbox to present the user benefits for using Brave VPN"
    )

    public static let popupCheckboxBlockAdsAlternate = NSLocalizedString(
      "vpn.popupCheckboxBlockAdsAlternate",
      tableName: "BraveShared",
      bundle: .module,
      value: "Block ads & trackers across all apps ",
      comment: "Text for a checkbox to present the user benefits for using Brave VPN"
    )

    public static let popupCheckboxFast = NSLocalizedString(
      "vpn.popupCheckboxFast",
      tableName: "BraveShared",
      bundle: .module,
      value: "Supports speeds of up to 100 Mbps",
      comment: "Text for a checkbox to present the user benefits for using Brave VPN"
    )

    public static let popupCheckboxFastAlternate = NSLocalizedString(
      "vpn.popupCheckboxFastAlternate",
      tableName: "BraveShared",
      bundle: .module,
      value: "Fast and unlimited up to 100 Mbps",
      comment: "Text for a checkbox to present the user benefits for using Brave VPN"
    )

    public static let popupCheckmarkSecureConnections = NSLocalizedString(
      "vpn.popupCheckmarkSecureConnections",
      tableName: "BraveShared",
      bundle: .module,
      value: "Secures all connections",
      comment: "Text for a checkbox to present the user benefits for using Brave VPN"
    )

    public static let popupCheckmark247Support = NSLocalizedString(
      "vpn.popupCheckmark247Support",
      tableName: "BraveShared",
      bundle: .module,
      value: "24/7 support",
      comment: "Text for a checkbox to present the user benefits for using Brave VPN"
    )

    public static let infoCheckPrivacy = NSLocalizedString(
      "vpn.infoCheckPrivacy",
      tableName: "BraveShared",
      bundle: .module,
      value: "Extra privacy & security online",
      comment: "Information text shown to users for using Brave VPN"
    )

    public static let infoCheckLocation = NSLocalizedString(
      "vpn.infoCheckLocation",
      tableName: "BraveShared",
      bundle: .module,
      value: "Hide your IP & change your location",
      comment: "Information Text shown to users for using Brave VPN"
    )

    public static let infoCheckServers = NSLocalizedString(
      "vpn.infoCheckServers",
      tableName: "BraveShared",
      bundle: .module,
      value: "Hundreds of servers around the world",
      comment: "Information Text shown to users for using Brave VPN"
    )

    public static let infoCheckConnectionSpeed = NSLocalizedString(
      "vpn.infoCheckConnectionSpeed",
      tableName: "BraveShared",
      bundle: .module,
      value: "Lightning-fast connection speeds",
      comment: "Information shown to users for using Brave VPN"
    )

    public static let infoCheckLimitDevice = NSLocalizedString(
      "vpn.infoCheckLimitDevice",
      tableName: "BraveShared",
      bundle: .module,
      value: "Protect up to 10 devices with one plan",
      comment: "Information shown to users for using Brave VPN"
    )

    public static let autoRenewSoonExpirePopOverTitle = NSLocalizedString(
      "vpn.autoRenewSoonExpireTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Oh no! Your Brave VPN subscription is about to expire.",
      comment: "Pop up title for VPN subscription is about expire"
    )

    public static let autoRenewDiscountPopOverTitle = NSLocalizedString(
      "vpn.autoRenewDiscountPopOverTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Auto-renew your Brave VPN Subscription now and get 20% off for 3 months!",
      comment: "Pop up title for renewing VPN subscription with discount"
    )

    public static let autoRenewFreeMonthPopOverTitle = NSLocalizedString(
      "vpn.autoRenewFreeMonthPopOverTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Auto-renew your Brave VPN Subscription now and get 1 month free!",
      comment: "Pop up title for renewing VPN subscription with month free"
    )

    public static let updateBillingSoonExpirePopOverTitle = NSLocalizedString(
      "vpn.updateBillingSoonExpirePopOverTitle",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "There's a billing issue with your account, which means your Brave VPN subscription is about to expire.",
      comment: "Pop up title for billing issue for subcription VPN about to expire"
    )

    public static let updateBillingExpiredPopOverTitle = NSLocalizedString(
      "vpn.updateBillingExpiredPopOverTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Update your payment info to stay protected with Brave VPN.",
      comment: "Pop up title for billing issue for subcription VPN already expired"
    )

    public static let autoRenewSoonExpirePopOverDescription = NSLocalizedString(
      "vpn.autoRenewSoonExpirePopOverDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "That means you'll lose Brave's extra protections for every app on your phone.",
      comment: "Pop up description for VPN subscription is about expire"
    )

    public static let updateBillingSoonExpirePopOverDescription = NSLocalizedString(
      "vpn.updateBillingSoonExpirePopOverDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "Want to keep protecting every app on your phone? Just update your payment details.",
      comment: "Pop up description for billing issue of subcription VPN about to expire"
    )

    public static let updateBillingExpiredPopOverDescription = NSLocalizedString(
      "vpn.updateBillingExpiredPopOverDescription",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "Don’t worry. We’ll keep VPN active for a few days while you are updating your payment info.",
      comment: "Pop up description for billing issue of subcription VPN already expired"
    )

    public static let subscribeVPNDiscountPopOverTitle = NSLocalizedString(
      "vpn.subscribeVPNDiscountPopOverTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Give Brave VPN another try and get 20% off for 3 months!",
      comment: "Pop up title for subscribing VPN with discount"
    )

    public static let subscribeVPNProtectionPopOverTitle = NSLocalizedString(
      "vpn.subscribeVPNProtectionPopOverTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Did you know that Brave VPN protects you outside of Brave Browser?",
      comment: "Pop up title for subscribing VPN explaning VPN protects user outside the Brave"
    )

    public static let subscribeVPNAllDevicesPopOverTitle = NSLocalizedString(
      "vpn.subscribeVPNAllDevicesPopOverTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Now, use Brave VPN on all your devices for the same price!",
      comment: "Pop up title the subscription for VPN can be used for all platforms"
    )

    public static let subscribeVPNProtectionPopOverDescription = NSLocalizedString(
      "vpn.subscribeVPNProtectionPopOverDescription",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "Brave VPN has always blocked trackers on every app, even outside the Brave browser. Now you can see who tried to track you, with the Brave Privacy Hub.",
      comment:
        "Pop up description for subscribing VPN explaning VPN protects user outside the Brave"
    )

    public static let subscribeVPNAllDevicesPopOverDescription = NSLocalizedString(
      "vpn.subscribeVPNAllDevicesPopOverDescription",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "That’s right. Your Brave VPN subscription is now good on up to 5 devices. So you can subscribe on iOS and use it on your Mac, Windows and Android devices for free.",
      comment: "Pop up description the subscription for VPN can be used for all platforms"
    )

    public static let subscribeVPNPopOverSubDescription = NSLocalizedString(
      "vpn.subscribeVPNPopOverSubDescription",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "Ready to safeguard every app on your phone? Come back to Brave VPN and get 20% off for the next 3 months.",
      comment: "Pop up sub description the subscription for VPN can be used for all platforms"
    )

    public static let sessionExpiredTitle = NSLocalizedString(
      "vpn.sessionExpiredTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Session expired",
      comment: "Alert title to show when the VPN session has expired"
    )

    public static let sessionExpiredDescription = NSLocalizedString(
      "vpn.sessionExpiredDescription",
      tableName: "BraveShared",
      bundle: .module,
      value: "Please login to your Brave Account to refresh your VPN session.",
      comment: "Alert description to show when the VPN session has expired"
    )

    public static let sessionExpiredLoginButton = NSLocalizedString(
      "vpn.sessionExpiredLoginButton",
      tableName: "BraveShared",
      bundle: .module,
      value: "Login",
      comment: "Login button to fix the vpn session expiration issue"
    )

    public static let sessionExpiredDismissButton = NSLocalizedString(
      "vpn.sessionExpiredDismissButton",
      tableName: "BraveShared",
      bundle: .module,
      value: "Dismiss",
      comment: "Dismiss button to close the alert showing that the vpn session has expired"
    )

    public static let vpnRegionSelectorButtonSubTitle = NSLocalizedString(
      "vpn.vpnRegionSelectorButtonSubTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Current Setting: %@",
      comment:
        "Button subtitle for VPN region selection in menu. %@ will be replaced with country name or automatic ex: Current Setting: Automatic"
    )

    public static let vpnUpdatePaymentMethodDescriptionText = NSLocalizedString(
      "vpn.vpnUpdatePaymentMethodDescriptionText",
      tableName: "BraveShared",
      bundle: .module,
      value: "Please update your payment method",
      comment: "Text description of VPN subscription needs a method of payment update"
    )

    public static let settingsVPNExpired = NSLocalizedString(
      "vpn.settingsVPNExpired",
      tableName: "BraveShared",
      bundle: .module,
      value: "Expired",
      comment: "Whether the VPN plan has expired"
    )

    public static let vpnRegionSelectorButtonTitle = NSLocalizedString(
      "vpn.vpnRegionSelectorButtonTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "VPN Region",
      comment: "Button title for VPN region selection in menu"
    )

    public static let regionPickerAutomaticModeCellText = NSLocalizedString(
      "vpn.regionPickerAutomaticModeCellText",
      tableName: "BraveShared",
      bundle: .module,
      value: "Automatic",
      comment: "Name of automatic vpn region selector"
    )

    public static let autoRenewActionButtonTitle = NSLocalizedString(
      "vpn.autoRenewActionButtonTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Enable Auto-Renew",
      comment: "Action button title that enables auto renew for subcription"
    )

    public static let autoReneSoonExpirePopOverSubDescription = NSLocalizedString(
      "vpn.autoReneSoonExpirePopOverSubDescription",
      tableName: "BraveShared",
      bundle: .module,
      value:
        "Want to stay protected? Just renew before your subscription ends. As a thanks for renewing, we'll even take 20% off for the next 3 months.",
      comment: "Pop up extra description for billing issue of subcription VPN about to expire"
    )

    public static let updatePaymentActionButtonTitle = NSLocalizedString(
      "vpn.updatePaymentActionButtonTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Update Payment",
      comment: "Action button title that updates method payment"
    )

    public static let subscribeVPNActionButtonTitle = NSLocalizedString(
      "vpn.subscribeVPNActionButtonTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Subscribe Now",
      comment: "Action button title that subscribe action for VPN purchase"
    )

    public static let vpnRegionListServerScreenTitle = NSLocalizedString(
      "vpn.vpnRegionListServerScreenTitle",
      tableName: "BraveShared",
      bundle: .module,
      value: "Server Region",
      comment: "The title for screen for showing list of server region"
    )

    public static let freeTrialDetail = NSLocalizedString(
      "vpn.freeTrialDetail",
      bundle: .module,
      value: "All plans include a",
      comment:
        "Used in context: All plans include a 'free 7-day trial'!"
    )

    public static let freeTrialPeriod = NSLocalizedString(
      "vpn.freeTrialPeriod",
      bundle: .module,
      value: "free 7-day trial",
      comment:
        "Used in context: All plans include a 'free 7-day trial'! - this will be the disclamier for the trial showing it is free and 7 days long"
    )

    public static let freeTrialPeriodAction = NSLocalizedString(
      "vpn.freeTrialPeriodAction",
      bundle: .module,
      value: "try 7 days free",
      comment: "The button text that starts the trial action"
    )

    public static let activateSubscriptionAction = NSLocalizedString(
      "vpn.activateSubscriptionAction",
      bundle: .module,
      value: "activate",
      comment: "The button text that starts the subscription action"
    )

    public static let restorePurchases = NSLocalizedString(
      "vpn.restorePurchases",
      bundle: .module,
      value: "Restore",
      comment: ""
    )

    public static let monthlySubTitle = NSLocalizedString(
      "vpn.monthlySubTitle",
      bundle: .module,
      value: "Monthly Subscription",
      comment: ""
    )

    public static let monthlySubDetail =
      NSLocalizedString(
        "vpn.monthlySubDetail",
        bundle: .module,
        value: "Renews monthly",
        comment: "Used in context: 'Monthly subscription, (it) renews monthly'"
      )

    public static let yearlySubTitle = NSLocalizedString(
      "vpn.yearlySubTitle",
      bundle: .module,
      value: "One Year",
      comment: "One year lenght vpn subcription"
    )

    public static let yearlySubDetail = NSLocalizedString(
      "vpn.yearlySubDetail",
      bundle: .module,
      value: "Renew annually save %@",
      comment:
        "Used in context: 'yearly subscription, renew annually (to) save 16%'. The placeholder is for percent value"
    )

    public static let yearlySubDisclaimer = NSLocalizedString(
      "vpn.yearlySubDisclaimer",
      bundle: .module,
      value: "BEST VALUE",
      comment:
        "It's like when there's few subscription plans, and one plan has the best value to price ratio, so this label says next to that plan: '(plan) - BEST VALUE'"
    )

    // MARK: Checkboxes

    public static let checkboxProtectYourDevices = NSLocalizedString(
      "vpn.checkboxProtectYourDevices",
      bundle: .module,
      value: "Protect every app & your whole device",
      comment: "Text for a checkbox to present the user benefits for using Brave VPN"
    )

    public static let checkboxSaferWifi = NSLocalizedString(
      "vpn.checkboxSaferWifi",
      bundle: .module,
      value: "Safer for home or public Wi-Fi",
      comment: "Text for a checkbox to present the user benefits for using Brave VPN"
    )

    public static let checkboxSpeedFast = NSLocalizedString(
      "vpn.checkboxSpeedFast",
      bundle: .module,
      value: "Lightning-fast, up to 100 Mbps",
      comment: "Text for a checkbox to present the user benefits for using Brave VPN"
    )

    public static let checkboxGeoLocation = NSLocalizedString(
      "vpn.checkboxGeoLocation",
      bundle: .module,
      value: "Choose your geo/country location",
      comment: "Text for a checkbox to present the user benefits for using Brave VPN"
    )

    public static let checkboxNoIPLog = NSLocalizedString(
      "vpn.checkboxNoIPLog",
      bundle: .module,
      value: "Brave never logs your activity",
      comment: "Text for a checkbox to present the user benefits for using Brave VPN"
    )

    public static let checkboxDevicesProtect = NSLocalizedString(
      "vpn.checkboxDevicesProtect",
      bundle: .module,
      value: "Protect 5 devices on 1 subscription",
      comment: "Text for a checkbox to present the user benefits for using Brave VPN"
    )

    public static let installTitle = NSLocalizedString(
      "vpn.installTitle",
      bundle: .module,
      value: "Install VPN",
      comment: "Title for screen to install the VPN."
    )

    public static let installProfileTitle = NSLocalizedString(
      "vpn.installProfileTitle",
      bundle: .module,
      value: "Brave will now install a VPN profile.",
      comment: ""
    )

    public static let installProfileBody = NSLocalizedString(
      "vpn.installProfileBody",
      bundle: .module,
      value:
        "This profile allows the VPN to automatically connect and secure traffic across your device all the time. This VPN connection will be encrypted and routed through Brave's intelligent firewall to block potentially harmful and invasive connections.",
      comment: "Text explaining how the VPN works."
    )

    public static let installProfileButtonText = NSLocalizedString(
      "vpn.installProfileButtonText",
      bundle: .module,
      value: "Install VPN Profile",
      comment: "Text for 'install vpn profile' button"
    )

    public static let settingsSubscriptionSection = NSLocalizedString(
      "vpn.settingsSubscriptionSection",
      bundle: .module,
      value: "Subscription",
      comment: "Header title for vpn settings subscription section."
    )

    public static let settingsServerSection = NSLocalizedString(
      "vpn.settingsServerSection",
      bundle: .module,
      value: "Server",
      comment: "Header title for vpn settings server section."
    )

    public static let settingsSubscriptionStatus = NSLocalizedString(
      "vpn.settingsSubscriptionStatus",
      bundle: .module,
      value: "Status",
      comment: "Table cell title for status of current VPN subscription."
    )

    public static let settingsSubscriptionExpiration = NSLocalizedString(
      "vpn.settingsSubscriptionExpiration",
      bundle: .module,
      value: "Expires",
      comment: "Table cell title for cell that shows when the VPN subscription expires."
    )

    public static let settingsManageSubscription = NSLocalizedString(
      "vpn.settingsManageSubscription",
      bundle: .module,
      value: "Manage Subscription",
      comment: "Button to manage your VPN subscription"
    )

    public static let settingsRedeemOfferCode = NSLocalizedString(
      "vpn.settingsRedeemOfferCode",
      bundle: .module,
      value: "Redeem Offer Code",
      comment: "Button to redeem offer code subscription"
    )

    public static let settingsLinkReceipt = NSLocalizedString(
      "vpn.settingsLinkReceipt",
      bundle: .module,
      value: "Link purchase to your Brave account",
      comment: "Button to link your VPN receipt to other devices."
    )

    public static let settingsLinkReceiptFooter = NSLocalizedString(
      "vpn.settingsLinkReceiptFooter",
      bundle: .module,
      value:
        "Link your App Store purchase to your Brave account to use Brave VPN on other devices.",
      comment: "Footer text to link your VPN receipt to other devices."
    )

    public static let settingsViewReceipt = NSLocalizedString(
      "vpn.settingsViewReceipt",
      bundle: .module,
      value: "View AppStore Receipt",
      comment: "Button to allow the user to view the app-store receipt."
    )

    public static let settingsServerHost = NSLocalizedString(
      "vpn.settingsServerHost",
      bundle: .module,
      value: "Host",
      comment: "Table cell title for vpn's server host"
    )

    public static let settingsServerLocation = NSLocalizedString(
      "vpn.settingsServerLocation",
      bundle: .module,
      value: "Location",
      comment: "Table cell title for vpn's server location and which open opens location select"
    )

    public static let settingsResetConfiguration = NSLocalizedString(
      "vpn.settingsResetConfiguration",
      bundle: .module,
      value: "Reset Configuration",
      comment: "Button to reset VPN configuration"
    )

    public static let settingsTransportProtocol = NSLocalizedString(
      "vpn.settingsTransportProtocol",
      bundle: .module,
      value: "Transport Protocol",
      comment:
        "Table cell title for vpn's transport protocol and which open opens protocol select"
    )

    public static let settingsContactSupport = NSLocalizedString(
      "vpn.settingsContactSupport",
      bundle: .module,
      value: "Contact Technical Support",
      comment: "Button to contact tech support"
    )

    public static let settingsFAQ = NSLocalizedString(
      "vpn.settingsFAQ",
      bundle: .module,
      value: "VPN Support",
      comment: "Button for FAQ"
    )

    public static let enableButton = NSLocalizedString(
      "vpn.enableButton",
      bundle: .module,
      value: "Enable",
      comment: "Button text to enable Brave VPN"
    )

    public static let buyButton = NSLocalizedString(
      "vpn.buyButton",
      bundle: .module,
      value: "Buy",
      comment: "Button text to buy Brave VPN"
    )

    public static let tryForFreeButton = NSLocalizedString(
      "vpn.learnMore",
      bundle: .module,
      value: "Start 7-Day Free Trial",
      comment: "Button text to try free Brave VPN"
    )

    public static let settingHeaderBody = NSLocalizedString(
      "vpn.settingHeaderBody",
      bundle: .module,
      value:
        "Protect every app with Brave VPN. Protect up to 10 devices with one plan.",
      comment: "VPN Banner Description"
    )

    public static let vpnConfigGenericErrorTitle = NSLocalizedString(
      "vpn.vpnConfigGenericErrorTitle",
      bundle: .module,
      value: "Error",
      comment: "Title for an alert when the VPN can't be configured"
    )

    public static let vpnConfigGenericErrorBody = NSLocalizedString(
      "vpn.vpnConfigGenericErrorBody",
      bundle: .module,
      value:
        "There was a problem initializing the VPN. Please try again or try resetting configuration in the VPN settings page.",
      comment: "Message for an alert when the VPN can't be configured."
    )

    public static let vpnConfigPermissionDeniedErrorTitle = NSLocalizedString(
      "vpn.vpnConfigPermissionDeniedErrorTitle",
      bundle: .module,
      value: "Permission denied",
      comment: "Title for an alert when the user didn't allow to install VPN profile"
    )

    public static let vpnRedeemCodeButtonActionTitle = NSLocalizedString(
      "vpn.vpnRedeemCodeButtonActionTitle",
      bundle: .module,
      value: "Redeem Code",
      comment: "Title for a button for enabling the Redeem Code flow"
    )

    public static let vpnConfigPermissionDeniedErrorBody = NSLocalizedString(
      "vpn.vpnConfigPermissionDeniedErrorBody",
      bundle: .module,
      value:
        "The Brave Firewall + VPN requires a VPN profile to be installed on your device to work. ",
      comment: "Title for an alert when the user didn't allow to install VPN profile"
    )

    public static let vpnSettingsMonthlySubName = NSLocalizedString(
      "vpn.vpnSettingsMonthlySubName",
      bundle: .module,
      value: "Monthly subscription",
      comment: "Name of monthly subscription in VPN Settings"
    )

    public static let vpnSettingsYearlySubName = NSLocalizedString(
      "vpn.vpnSettingsYearlySubName",
      bundle: .module,
      value: "Yearly subscription",
      comment: "Name of annual subscription in VPN Settings"
    )

    public static let vpnErrorPurchaseFailedTitle = NSLocalizedString(
      "vpn.vpnErrorPurchaseFailedTitle",
      bundle: .module,
      value: "Error",
      comment: "Title for error when VPN could not be purchased."
    )

    public static let vpnErrorPurchaseFailedBody = NSLocalizedString(
      "vpn.vpnErrorPurchaseFailedBody",
      bundle: .module,
      value:
        "Unable to complete purchase. Please try again, or check your payment details on Apple and try again.",
      comment: "Message for error when VPN could not be purchased."
    )

    public static let vpnErrorOfferCodeFailedBody = NSLocalizedString(
      "vpn.vpnErrorOfferCodeFailedBody",
      bundle: .module,
      value:
        "Unable to redeem offer code. Please try again, or check your offer code details and try again.",
      comment: "Message for error when VPN offer code could not be redeemed."
    )

    public static let vpnResetAlertTitle = NSLocalizedString(
      "vpn.vpnResetAlertTitle",
      bundle: .module,
      value: "Reset configuration",
      comment: "Title for alert to reset vpn configuration"
    )

    public static let vpnResetAlertBody = NSLocalizedString(
      "vpn.vpnResetAlertBody",
      bundle: .module,
      value:
        "This will reset your Brave Firewall + VPN configuration and fix any errors. This process may take a minute.",
      comment: "Message for alert to reset vpn configuration"
    )

    public static let vpnResetButton = NSLocalizedString(
      "vpn.vpnResetButton",
      bundle: .module,
      value: "Reset",
      comment: "Button name to reset vpn configuration"
    )

    public static let contactFormHostname = NSLocalizedString(
      "vpn.contactFormHostname",
      bundle: .module,
      value: "VPN Hostname",
      comment: "VPN Hostname field for customer support contact form."
    )

    public static let contactFormSubscriptionType = NSLocalizedString(
      "vpn.contactFormSubscriptionType",
      bundle: .module,
      value: "Subscription Type",
      comment: "Subscription Type field for customer support contact form."
    )

    public static let contactFormAppStoreReceipt = NSLocalizedString(
      "vpn.contactFormAppStoreReceipt",
      bundle: .module,
      value: "App Store Receipt",
      comment: "App Store Receipt field for customer support contact form."
    )

    public static let contactFormAppVersion = NSLocalizedString(
      "vpn.contactFormAppVersion",
      bundle: .module,
      value: "App Version",
      comment: "App Version field for customer support contact form."
    )

    public static let contactFormTimezone = NSLocalizedString(
      "vpn.contactFormTimezone",
      bundle: .module,
      value: "iOS Timezone",
      comment: "iOS Timezone field for customer support contact form."
    )

    public static let contactFormPlatform = NSLocalizedString(
      "vpn.contactFormPlatform",
      bundle: .module,
      value: "Platform",
      comment: "A contact form field that displays platform type: 'iOS' 'Android' or 'Desktop'"
    )

    public static let contactFormNetworkType = NSLocalizedString(
      "vpn.contactFormNetworkType",
      bundle: .module,
      value: "Network Type",
      comment: "Network Type field for customer support contact form."
    )

    public static let contactFormCarrier = NSLocalizedString(
      "vpn.contactFormCarrier",
      bundle: .module,
      value: "Cellular Carrier",
      comment: "Cellular Carrier field for customer support contact form."
    )

    public static let contactFormLogs = NSLocalizedString(
      "vpn.contactFormLogs",
      bundle: .module,
      value: "Error Logs",
      comment: "VPN logs field for customer support contact form."
    )

    public static let contactFormIssue = NSLocalizedString(
      "vpn.contactFormIssue",
      bundle: .module,
      value: "Issue",
      comment: "Specific issue field for customer support contact form."
    )

    public static let contactFormIssueDescription = NSLocalizedString(
      "vpn.contactFormIssueDescription",
      bundle: .module,
      value: "Please choose the category that describes the issue.",
      comment: "Description used for specific issue field for customer support contact form."
    )

    public static let contactFormFooterSharedWithGuardian = NSLocalizedString(
      "vpn.contactFormFooterSharedWithGuardian",
      bundle: .module,
      value: "Support provided with the help of the Guardian team.",
      comment: "Footer for customer support contact form."
    )

    public static let contactFormFooter = NSLocalizedString(
      "vpn.contactFormFooter",
      bundle: .module,
      value:
        "Please select the information you're comfortable sharing with us.\n\nThe more information you initially share with us the easier it will be for our support staff to help you resolve your issue.",
      comment: "Footer for customer support contact form."
    )

    public static let contactFormSendButton = NSLocalizedString(
      "vpn.contactFormSendButton",
      bundle: .module,
      value: "Continue to Email",
      comment: "Button name to send contact form."
    )

    public static let contactFormIssueOtherConnectionError = NSLocalizedString(
      "vpn.contactFormIssueOtherConnectionError",
      bundle: .module,
      value: "Cannot connect to the VPN (Other error)",
      comment: "Other connection problem for contact form issue field."
    )

    public static let contactFormIssueNoInternet =
      NSLocalizedString(
        "vpn.contactFormIssueNoInternet",
        bundle: .module,
        value: "No internet when connected",
        comment: "No internet problem for contact form issue field."
      )

    public static let contactFormIssueSlowConnection = NSLocalizedString(
      "vpn.contactFormIssueSlowConnection",
      bundle: .module,
      value: "Slow connection",
      comment: "Slow connection problem for contact form issue field."
    )

    public static let contactFormIssueWebsiteProblems = NSLocalizedString(
      "vpn.contactFormIssueWebsiteProblems",
      bundle: .module,
      value: "Website doesn't work",
      comment: "Website problem for contact form issue field."
    )

    public static let contactFormIssueConnectionReliability = NSLocalizedString(
      "vpn.contactFormIssueConnectionReliability",
      bundle: .module,
      value: "Connection reliability problem",
      comment: "Connection problems for contact form issue field."
    )

    public static let contactFormIssueOther = NSLocalizedString(
      "vpn.contactFormIssueOther",
      bundle: .module,
      value: "Other",
      comment: "Other problem for contact form issue field."
    )

    public static let subscriptionStatusExpired = NSLocalizedString(
      "vpn.planExpired",
      bundle: .module,
      value: "Expired",
      comment: "Text to show user when their vpn plan has expired"
    )

    public static let resetVPNErrorTitle = NSLocalizedString(
      "vpn.resetVPNErrorTitle",
      bundle: .module,
      value: "Reset Failed",
      comment: "Title for error message when vpn configuration reset fails."
    )

    public static let resetVPNErrorBody = NSLocalizedString(
      "vpn.resetVPNErrorBody",
      bundle: .module,
      value:
        "Unable to reset VPN configuration. Please try again. If the issue persists, contact support for assistance.",
      comment: "Message to show when vpn configuration reset fails."
    )

    public static let resetVPNErrorButtonActionTitle = NSLocalizedString(
      "vpn.resetVPNErrorButtonActionTitle",
      bundle: .module,
      value: "Try Again",
      comment: "Title of button to try again when vpn configuration reset fails."
    )

    public static let resetVPNSuccessTitle = NSLocalizedString(
      "vpn.resetVPNSuccessTitle",
      bundle: .module,
      value: "Reset Successful",
      comment: "Title for success message when vpn configuration reset succeeds."
    )

    public static let resetVPNSuccessBody = NSLocalizedString(
      "vpn.resetVPNSuccessBody",
      bundle: .module,
      value: "VPN configuration has been reset successfully.",
      comment: "Message to show when vpn configuration reset succeeds."
    )

    public static let contactFormDoNotEditText = NSLocalizedString(
      "vpn.contactFormDoNotEditText",
      bundle: .module,
      value:
        "Brave doesn’t track you or know how you use our app, so we don’t know how you've set up VPN. Please share info about the issue you're experiencing and we'll do our best to resolve it as soon as we can.",
      comment: "Text to tell user to not modify support info below email's body."
    )

    public static let contactFormTitle = NSLocalizedString(
      "vpn.contactFormTitle",
      bundle: .module,
      value: "Brave Firewall + VPN Issue",
      comment: "Title for contact form email."
    )

    public static let iapDisclaimer = NSLocalizedString(
      "vpn.iapDisclaimer",
      bundle: .module,
      value: "All subscriptions are auto-renewed but can be cancelled before renewal.",
      comment: "Disclaimer about in app subscription"
    )

    public static let installSuccessPopup = NSLocalizedString(
      "vpn.installSuccessPopup",
      bundle: .module,
      value: "VPN is now enabled",
      comment: "Popup that shows after user installs the vpn for the first time."
    )

    public static let vpnBackgroundNotificationTitle = NSLocalizedString(
      "vpn.vpnBackgroundNotificationTitle",
      bundle: .module,
      value: "Brave Firewall + VPN is ON",
      comment: "Notification title to tell user that the vpn is turned on even in background"
    )

    public static let vpnBackgroundNotificationBody = NSLocalizedString(
      "vpn.vpnBackgroundNotificationBody",
      bundle: .module,
      value: "Even in the background, Brave will continue to protect you.",
      comment: "Notification title to tell user that the vpn is turned on even in background"
    )

    public static let vpnIAPBoilerPlate = NSLocalizedString(
      "vpn.vpnIAPBoilerPlate",
      bundle: .module,
      value:
        "Subscriptions will be charged via your iTunes account.\n\nAny unused portion of the free trial, if offered, is forfeited when you buy a subscription.\n\nYour subscription will renew automatically unless it is cancelled at least 24 hours before the end of the current period.\n\nYou can manage your subscriptions in Settings.\n\nBy using Brave, you agree to the Terms of Use and Privacy Policy.",
      comment: "Disclaimer for user purchasing the VPN plan."
    )

    public static let regionPickerTitle = NSLocalizedString(
      "vpn.regionPickerTitle",
      bundle: .module,
      value: "Server Region",
      comment: "Title for vpn region selector screen"
    )

    public static let regionPickerAutomaticDescription = NSLocalizedString(
      "vpn.regionPickerAutomaticDescription",
      bundle: .module,
      value:
        "A server region most proximate to you will be automatically selected, based on your system timezone. This is recommended in order to ensure fast internet speeds.",
      comment: "Description of what automatic server selection does."
    )

    public static let regionPickerErrorTitle = NSLocalizedString(
      "vpn.regionPickerErrorTitle",
      bundle: .module,
      value: "Server Error",
      comment: "Title for error when we fail to switch vpn server for the user"
    )

    public static let regionPickerErrorMessage = NSLocalizedString(
      "vpn.regionPickerErrorMessage",
      bundle: .module,
      value: "Failed to switch servers, please try again later.",
      comment: "Message for error when we fail to switch vpn server for the user"
    )

    public static let protocolPickerTitle = NSLocalizedString(
      "vpn.protocolPickerTitle",
      bundle: .module,
      value: "Transport Protocol",
      comment: "Title for vpn tunnel protocol screen"
    )

    public static let protocolPickerDescription = NSLocalizedString(
      "vpn.protocolPickerDescription",
      bundle: .module,
      value:
        "Please select your preferred transport protocol. Once switched your existing VPN credentials will be cleared and you will be reconnected if a VPN connection is currently established.",
      comment: "Description of vpn tunnel protocol"
    )

    public static let regionSwitchSuccessPopupText = NSLocalizedString(
      "vpn.regionSwitchSuccessPopupText",
      bundle: .module,
      value: "VPN region changed.",
      comment: "Message that we show after successfully changing vpn region."
    )

    public static let protocolPickerErrorTitle =
      NSLocalizedString(
        "vpn.protocolPickerErrorTitle",
        bundle: .module,
        value: "Server Error",
        comment: "Title for error when we fail to switch tunnel protocol for the user"
      )

    public static let protocolPickerErrorMessage = NSLocalizedString(
      "vpn.protocolPickerErrorMessage",
      bundle: .module,
      value: "Failed to switch tunnel protocol, please try again later.",
      comment: "Message for error when we fail to switch tunnel protocol for the user"
    )

    public static let protocolSwitchSuccessPopupText = NSLocalizedString(
      "vpn.protocolSwitchSuccessPopupText",
      bundle: .module,
      value: "VPN Tunnel Protocol changed.",
      comment: "Message that we show after successfully changing tunnel protocol."
    )

    public static let settingsFailedToFetchServerList = NSLocalizedString(
      "vpn.settingsFailedToFetchServerList",
      bundle: .module,
      value: "Failed to retrieve server list, please try again later.",
      comment: "Error message shown if we failed to retrieve vpn server list."
    )

    public static let contactFormEmailNotConfiguredBody = NSLocalizedString(
      "vpn.contactFormEmailNotConfiguredBody",
      bundle: .module,
      value: "Can't send email. Please check your email configuration.",
      comment: "Button name to send contact form."
    )

    public static let vpnActionUpdatePaymentMethodSettingsText = NSLocalizedString(
      "vpn.vpnActionUpdatePaymentMethodSettingsText",
      bundle: .module,
      value: "Update Payment Method",
      comment:
        "Text for necesseray required action of VPN subscription needs a method of payment update"
    )

    public static let vpnCityRegionOptimalTitle = NSLocalizedString(
      "vpn.vpnCityRegionOptimalTitle",
      bundle: .module,
      value: "Optimal",
      comment:
        "Text for title of list item which chooses the optimal server for the country region"
    )

    public static let vpnCityRegionOptimalDescription =
      NSLocalizedString(
        "vpn.vpnCityRegionOptimalDescription",
        bundle: .module,
        value: "Use the best server available",
        comment:
          "Text for description of list item which chooses the optimal server for the country region"
      )

    public static let serverRegionAutoSelectDescription = NSLocalizedString(
      "vpn.serverRegionAutoSelectDescription",
      bundle: .module,
      value:
        "Auto-select the VPN server region closest to you based on your timezone. This option is recommended to maximize Internet speeds.",
      comment:
        "Text for description of toggle that auto selcets the server according to time zone."
    )

    public static let cityCountTitle = NSLocalizedString(
      "vpn.cityCountTitle",
      bundle: .module,
      value: "%ld City",
      comment: "Text indicating how many cities is used for that country used as Ex: '1 City'"
    )

    public static let multipleCityCountTitle = NSLocalizedString(
      "vpn.multipleCityCountTitle",
      bundle: .module,
      value: "%ld Cities",
      comment: "Text indicating how many cities is used for that region used as Ex: '2 Cities'"
    )

    public static let serverCountTitle = NSLocalizedString(
      "vpn.serverTitle",
      bundle: .module,
      value: "%ld Server",
      comment: "Text indicating how many server is used for that region used as Ex: '1 Server'"
    )

    public static let multipleServerCountTitle = NSLocalizedString(
      "vpn.multipleServerCountTitle",
      bundle: .module,
      value: "%ld Servers",
      comment: "Text indicating how many server is used for that region used as Ex: '2 Servers'"
    )

    public static let connectedRegionDescription = NSLocalizedString(
      "vpn.connectedRegionDescription",
      bundle: .module,
      value: "Connected",
      comment: "Text showing the vpn region is connected"
    )

    public static let automaticServerSelectionToggleTitle = NSLocalizedString(
      "vpn.automaticServerSelectionToggleTitle",
      bundle: .module,
      value: "Automatic",
      comment:
        "Text for toggle which indicating the toggle action is for selecting region in automatic"
    )

    public static let availableServerTitle = NSLocalizedString(
      "vpn.availableServerTitle",
      bundle: .module,
      value: "AVAILABLE SERVERS",
      comment: "Title on top the list of available servers"
    )

    public static let serverNameTitle = NSLocalizedString(
      "vpn.serverNameTitle",
      bundle: .module,
      value: "%@ Server",
      comment: "Title showing which country serves list showing. Ex: 'Brazil Server"
    )

    public static let vpnRegionChangedTitle = NSLocalizedString(
      "vpn.vpnRegionChangedTitle",
      bundle: .module,
      value: "VPN Region Changed",
      comment: "The alert title showing vpn region is changed"
    )

    public static let paywallYearlyPriceDividend = NSLocalizedString(
      "vpn.paywallYearlyPriceDividend",
      bundle: .module,
      value: "year",
      comment: "The text which will be used to indicate period of payments like 150 / year"
    )

    public static let paywallMonthlyPriceDividend = NSLocalizedString(
      "vpn.paywallMonthlyPriceDividend",
      bundle: .module,
      value: "month",
      comment: "The text which will be used to indicate period of payments like 10 / month"
    )

    public static let renewAnnually = NSLocalizedString(
      "vpn.renewAnnually",
      bundle: .module,
      value: "Renews annually",
      comment: "Yearly tier explaination. Subscription will be renewed annually."
    )

    public static let renewMonthly = NSLocalizedString(
      "vpn.renewMonthly",
      bundle: .module,
      value: "Renews monthly",
      comment: "Monthly tier explaination. Subscription will be renewed monthly."
    )

    public static let save = NSLocalizedString(
      "vpn.save",
      bundle: .module,
      value: "save",
      comment: "Use in context: save 16%."
    )

    public static let paywallDisclaimer = NSLocalizedString(
      "vpn.paywallDisclaimer",
      bundle: .module,
      value:
        "Subscriptions will be charged via your Apple account. Any unused portion of the free trial, if offered, is forfeited when you buy a subscription.\n\nYour subscription will renew automatically unless it is canceled at least 24 hours before the end of the current period.\n\nYou can manage your subscriptions in Settings.\n\nBy using Brave, you agree to the Terms of Use and Privacy Policy.",
      comment:
        "The text that briefly explains how Brave VPN subscription is going to be charged and managed."
    )
  }
}
