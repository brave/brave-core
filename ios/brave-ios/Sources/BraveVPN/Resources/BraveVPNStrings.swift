// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Strings

extension Strings {
  struct VPN {
    public static let freeTrialDetail =
      NSLocalizedString(
        "vpn.freeTrialDetail",
        tableName: "BraveShared",
        bundle: .module,
        value: "All plans include a %@!",
        comment:
          "Used in context: All plans include a 'free 7-day trial'! where variable part will be indicating what kind of trial it will include"
      )

    public static let freeTrialPeriod =
      NSLocalizedString(
        "vpn.freeTrialPeriod",
        tableName: "BraveShared",
        bundle: .module,
        value: "free 7-day trial",
        comment:
          "Used in context: All plans include a 'free 7-day trial'! - this will be the disclamier for the trial showing it is free and 7 days long"
      )

    public static let freeTrialPeriodAction =
      NSLocalizedString(
        "vpn.freeTrialPeriodAction",
        tableName: "BraveShared",
        bundle: .module,
        value: "try 7 days free",
        comment: "The button text that starts the trial action"
      )

    public static let activateSubscriptionAction =
      NSLocalizedString(
        "vpn.activateSubscriptionAction",
        tableName: "BraveShared",
        bundle: .module,
        value: "activate",
        comment: "The button text that starts the subscription action"
      )

    public static let restorePurchases =
      NSLocalizedString(
        "vpn.restorePurchases",
        tableName: "BraveShared",
        bundle: .module,
        value: "Restore",
        comment: ""
      )

    public static let monthlySubTitle =
      NSLocalizedString(
        "vpn.monthlySubTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Monthly Subscription",
        comment: ""
      )

    public static let monthlySubDetail =
      NSLocalizedString(
        "vpn.monthlySubDetail",
        tableName: "BraveShared",
        bundle: .module,
        value: "Renews monthly",
        comment: "Used in context: 'Monthly subscription, (it) renews monthly'"
      )

    public static let yearlySubTitle =
      NSLocalizedString(
        "vpn.yearlySubTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "One year",
        comment: "One year lenght vpn subcription"
      )

    public static let yearlySubDetail =
      NSLocalizedString(
        "vpn.yearlySubDetail",
        tableName: "BraveShared",
        bundle: .module,
        value: "Renew annually save %@",
        comment:
          "Used in context: 'yearly subscription, renew annually (to) save 16%'. The placeholder is for percent value"
      )

    public static let yearlySubDisclaimer =
      NSLocalizedString(
        "vpn.yearlySubDisclaimer",
        tableName: "BraveShared",
        bundle: .module,
        value: "Best value",
        comment:
          "It's like when there's few subscription plans, and one plan has the best value to price ratio, so this label says next to that plan: '(plan) - Best value'"
      )

    // MARK: Checkboxes

    public static let checkboxProtectYourDevices =
      NSLocalizedString(
        "vpn.checkboxProtectYourDevices",
        tableName: "BraveShared",
        bundle: .module,
        value: "Protect every app & your whole device",
        comment: "Text for a checkbox to present the user benefits for using Brave VPN"
      )

    public static let checkboxSaferWifi =
      NSLocalizedString(
        "vpn.checkboxSaferWifi",
        tableName: "BraveShared",
        bundle: .module,
        value: "Safer for home or public Wi-Fi",
        comment: "Text for a checkbox to present the user benefits for using Brave VPN"
      )

    public static let checkboxSpeedFast =
      NSLocalizedString(
        "vpn.checkboxSpeedFast",
        tableName: "BraveShared",
        bundle: .module,
        value: "Lightning-fast, up to 100 Mbps",
        comment: "Text for a checkbox to present the user benefits for using Brave VPN"
      )

    public static let checkboxGeoLocation =
      NSLocalizedString(
        "vpn.checkboxGeoLocation",
        tableName: "BraveShared",
        bundle: .module,
        value: "Choose your geo/country location",
        comment: "Text for a checkbox to present the user benefits for using Brave VPN"
      )

    public static let checkboxNoIPLog =
      NSLocalizedString(
        "vpn.checkboxNoIPLog",
        tableName: "BraveShared",
        bundle: .module,
        value: "Brave never logs your activity",
        comment: "Text for a checkbox to present the user benefits for using Brave VPN"
      )

    public static let checkboxDevicesProtect =
      NSLocalizedString(
        "vpn.checkboxDevicesProtect",
        tableName: "BraveShared",
        bundle: .module,
        value: "Protect 5 devices on 1 subscription",
        comment: "Text for a checkbox to present the user benefits for using Brave VPN"
      )

    public static let installTitle =
      NSLocalizedString(
        "vpn.installTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Install VPN",
        comment: "Title for screen to install the VPN."
      )

    public static let installProfileTitle =
      NSLocalizedString(
        "vpn.installProfileTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Brave will now install a VPN profile.",
        comment: ""
      )

    public static let installProfileBody =
      NSLocalizedString(
        "vpn.installProfileBody",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "This profile allows the VPN to automatically connect and secure traffic across your device all the time. This VPN connection will be encrypted and routed through Brave's intelligent firewall to block potentially harmful and invasive connections.",
        comment: "Text explaining how the VPN works."
      )

    public static let installProfileButtonText =
      NSLocalizedString(
        "vpn.installProfileButtonText",
        tableName: "BraveShared",
        bundle: .module,
        value: "Install VPN Profile",
        comment: "Text for 'install vpn profile' button"
      )

    public static let settingsSubscriptionSection =
      NSLocalizedString(
        "vpn.settingsSubscriptionSection",
        tableName: "BraveShared",
        bundle: .module,
        value: "Subscription",
        comment: "Header title for vpn settings subscription section."
      )

    public static let settingsServerSection =
      NSLocalizedString(
        "vpn.settingsServerSection",
        tableName: "BraveShared",
        bundle: .module,
        value: "Server",
        comment: "Header title for vpn settings server section."
      )

    public static let settingsSubscriptionStatus =
      NSLocalizedString(
        "vpn.settingsSubscriptionStatus",
        tableName: "BraveShared",
        bundle: .module,
        value: "Status",
        comment: "Table cell title for status of current VPN subscription."
      )

    public static let settingsSubscriptionExpiration =
      NSLocalizedString(
        "vpn.settingsSubscriptionExpiration",
        tableName: "BraveShared",
        bundle: .module,
        value: "Expires",
        comment: "Table cell title for cell that shows when the VPN subscription expires."
      )

    public static let settingsManageSubscription =
      NSLocalizedString(
        "vpn.settingsManageSubscription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Manage Subscription",
        comment: "Button to manage your VPN subscription"
      )

    public static let settingsRedeemOfferCode =
      NSLocalizedString(
        "vpn.settingsRedeemOfferCode",
        tableName: "BraveShared",
        bundle: .module,
        value: "Redeem Offer Code",
        comment: "Button to redeem offer code subscription"
      )

    public static let settingsLinkReceipt =
      NSLocalizedString(
        "vpn.settingsLinkReceipt",
        tableName: "BraveShared",
        bundle: .module,
        value: "Link purchase to your Brave account",
        comment: "Button to link your VPN receipt to other devices."
      )

    public static let settingsLinkReceiptFooter =
      NSLocalizedString(
        "vpn.settingsLinkReceiptFooter",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Link your App Store purchase to your Brave account to use Brave VPN on other devices.",
        comment: "Footer text to link your VPN receipt to other devices."
      )

    public static let settingsServerHost =
      NSLocalizedString(
        "vpn.settingsServerHost",
        tableName: "BraveShared",
        bundle: .module,
        value: "Host",
        comment: "Table cell title for vpn's server host"
      )

    public static let settingsServerLocation =
      NSLocalizedString(
        "vpn.settingsServerLocation",
        tableName: "BraveShared",
        bundle: .module,
        value: "Location",
        comment: "Table cell title for vpn's server location and which open opens location select"
      )

    public static let settingsResetConfiguration =
      NSLocalizedString(
        "vpn.settingsResetConfiguration",
        tableName: "BraveShared",
        bundle: .module,
        value: "Reset Configuration",
        comment: "Button to reset VPN configuration"
      )

    public static let settingsTransportProtocol =
      NSLocalizedString(
        "vpn.settingsTransportProtocol",
        tableName: "BraveShared",
        bundle: .module,
        value: "Transport Protocol",
        comment:
          "Table cell title for vpn's transport protocol and which open opens protocol select"
      )

    public static let settingsContactSupport =
      NSLocalizedString(
        "vpn.settingsContactSupport",
        tableName: "BraveShared",
        bundle: .module,
        value: "Contact Technical Support",
        comment: "Button to contact tech support"
      )

    public static let settingsFAQ =
      NSLocalizedString(
        "vpn.settingsFAQ",
        tableName: "BraveShared",
        bundle: .module,
        value: "VPN Support",
        comment: "Button for FAQ"
      )

    public static let enableButton =
      NSLocalizedString(
        "vpn.enableButton",
        tableName: "BraveShared",
        bundle: .module,
        value: "Enable",
        comment: "Button text to enable Brave VPN"
      )

    public static let buyButton =
      NSLocalizedString(
        "vpn.buyButton",
        tableName: "BraveShared",
        bundle: .module,
        value: "Buy",
        comment: "Button text to buy Brave VPN"
      )

    public static let tryForFreeButton =
      NSLocalizedString(
        "vpn.learnMore",
        tableName: "BraveShared",
        bundle: .module,
        value: "Try for FREE",
        comment: "Button text to try free Brave VPN"
      )

    public static let settingHeaderBody =
      NSLocalizedString(
        "vpn.settingHeaderBody",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Upgrade to a VPN to protect your connection and block invasive trackers everywhere.",
        comment: "VPN Banner Description"
      )

    public static let vpnConfigGenericErrorTitle =
      NSLocalizedString(
        "vpn.vpnConfigGenericErrorTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Error",
        comment: "Title for an alert when the VPN can't be configured"
      )

    public static let vpnConfigGenericErrorBody =
      NSLocalizedString(
        "vpn.vpnConfigGenericErrorBody",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "There was a problem initializing the VPN. Please try again or try resetting configuration in the VPN settings page.",
        comment: "Message for an alert when the VPN can't be configured."
      )

    public static let vpnConfigPermissionDeniedErrorTitle =
      NSLocalizedString(
        "vpn.vpnConfigPermissionDeniedErrorTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Permission denied",
        comment: "Title for an alert when the user didn't allow to install VPN profile"
      )

    public static let vpnRedeemCodeButtonActionTitle =
      NSLocalizedString(
        "vpn.vpnRedeemCodeButtonActionTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Redeem Code",
        comment: "Title for a button for enabling the Redeem Code flow"
      )

    public static let vpnConfigPermissionDeniedErrorBody =
      NSLocalizedString(
        "vpn.vpnConfigPermissionDeniedErrorBody",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "The Brave Firewall + VPN requires a VPN profile to be installed on your device to work. ",
        comment: "Title for an alert when the user didn't allow to install VPN profile"
      )

    public static let vpnSettingsMonthlySubName =
      NSLocalizedString(
        "vpn.vpnSettingsMonthlySubName",
        tableName: "BraveShared",
        bundle: .module,
        value: "Monthly subscription",
        comment: "Name of monthly subscription in VPN Settings"
      )

    public static let vpnSettingsYearlySubName =
      NSLocalizedString(
        "vpn.vpnSettingsYearlySubName",
        tableName: "BraveShared",
        bundle: .module,
        value: "Yearly subscription",
        comment: "Name of annual subscription in VPN Settings"
      )

    public static let vpnErrorPurchaseFailedTitle =
      NSLocalizedString(
        "vpn.vpnErrorPurchaseFailedTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Error",
        comment: "Title for error when VPN could not be purchased."
      )

    public static let vpnErrorPurchaseFailedBody =
      NSLocalizedString(
        "vpn.vpnErrorPurchaseFailedBody",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Unable to complete purchase. Please try again, or check your payment details on Apple and try again.",
        comment: "Message for error when VPN could not be purchased."
      )

    public static let vpnErrorOfferCodeFailedBody =
      NSLocalizedString(
        "vpn.vpnErrorOfferCodeFailedBody",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Unable to redeem offer code. Please try again, or check your offer code details and try again.",
        comment: "Message for error when VPN offer code could not be redeemed."
      )

    public static let vpnResetAlertTitle =
      NSLocalizedString(
        "vpn.vpnResetAlertTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Reset configuration",
        comment: "Title for alert to reset vpn configuration"
      )

    public static let vpnResetAlertBody =
      NSLocalizedString(
        "vpn.vpnResetAlertBody",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "This will reset your Brave Firewall + VPN configuration and fix any errors. This process may take a minute.",
        comment: "Message for alert to reset vpn configuration"
      )

    public static let vpnResetButton =
      NSLocalizedString(
        "vpn.vpnResetButton",
        tableName: "BraveShared",
        bundle: .module,
        value: "Reset",
        comment: "Button name to reset vpn configuration"
      )

    public static let contactFormHostname =
      NSLocalizedString(
        "vpn.contactFormHostname",
        tableName: "BraveShared",
        bundle: .module,
        value: "VPN Hostname",
        comment: "VPN Hostname field for customer support contact form."
      )

    public static let contactFormSubscriptionType =
      NSLocalizedString(
        "vpn.contactFormSubscriptionType",
        tableName: "BraveShared",
        bundle: .module,
        value: "Subscription Type",
        comment: "Subscription Type field for customer support contact form."
      )

    public static let contactFormAppStoreReceipt =
      NSLocalizedString(
        "vpn.contactFormAppStoreReceipt",
        tableName: "BraveShared",
        bundle: .module,
        value: "App Store Receipt",
        comment: "App Store Receipt field for customer support contact form."
      )

    public static let contactFormAppVersion =
      NSLocalizedString(
        "vpn.contactFormAppVersion",
        tableName: "BraveShared",
        bundle: .module,
        value: "App Version",
        comment: "App Version field for customer support contact form."
      )

    public static let contactFormTimezone =
      NSLocalizedString(
        "vpn.contactFormTimezone",
        tableName: "BraveShared",
        bundle: .module,
        value: "iOS Timezone",
        comment: "iOS Timezone field for customer support contact form."
      )

    public static let contactFormPlatform =
      NSLocalizedString(
        "vpn.contactFormPlatform",
        tableName: "BraveShared",
        bundle: .module,
        value: "Platform",
        comment: "A contact form field that displays platform type: 'iOS' 'Android' or 'Desktop'"
      )

    public static let contactFormNetworkType =
      NSLocalizedString(
        "vpn.contactFormNetworkType",
        tableName: "BraveShared",
        bundle: .module,
        value: "Network Type",
        comment: "Network Type field for customer support contact form."
      )

    public static let contactFormCarrier =
      NSLocalizedString(
        "vpn.contactFormCarrier",
        tableName: "BraveShared",
        bundle: .module,
        value: "Cellular Carrier",
        comment: "Cellular Carrier field for customer support contact form."
      )

    public static let contactFormLogs =
      NSLocalizedString(
        "vpn.contactFormLogs",
        tableName: "BraveShared",
        bundle: .module,
        value: "Error Logs",
        comment: "VPN logs field for customer support contact form."
      )

    public static let contactFormIssue =
      NSLocalizedString(
        "vpn.contactFormIssue",
        tableName: "BraveShared",
        bundle: .module,
        value: "Issue",
        comment: "Specific issue field for customer support contact form."
      )

    public static let contactFormIssueDescription =
      NSLocalizedString(
        "vpn.contactFormIssueDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Please choose the category that describes the issue.",
        comment: "Description used for specific issue field for customer support contact form."
      )

    public static let contactFormFooterSharedWithGuardian =
      NSLocalizedString(
        "vpn.contactFormFooterSharedWithGuardian",
        tableName: "BraveShared",
        bundle: .module,
        value: "Support provided with the help of the Guardian team.",
        comment: "Footer for customer support contact form."
      )

    public static let contactFormFooter =
      NSLocalizedString(
        "vpn.contactFormFooter",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Please select the information you're comfortable sharing with us.\n\nThe more information you initially share with us the easier it will be for our support staff to help you resolve your issue.",
        comment: "Footer for customer support contact form."
      )

    public static let contactFormSendButton =
      NSLocalizedString(
        "vpn.contactFormSendButton",
        tableName: "BraveShared",
        bundle: .module,
        value: "Continue to Email",
        comment: "Button name to send contact form."
      )

    public static let contactFormIssueOtherConnectionError =
      NSLocalizedString(
        "vpn.contactFormIssueOtherConnectionError",
        tableName: "BraveShared",
        bundle: .module,
        value: "Cannot connect to the VPN (Other error)",
        comment: "Other connection problem for contact form issue field."
      )

    public static let contactFormIssueNoInternet =
      NSLocalizedString(
        "vpn.contactFormIssueNoInternet",
        tableName: "BraveShared",
        bundle: .module,
        value: "No internet when connected",
        comment: "No internet problem for contact form issue field."
      )

    public static let contactFormIssueSlowConnection =
      NSLocalizedString(
        "vpn.contactFormIssueSlowConnection",
        tableName: "BraveShared",
        bundle: .module,
        value: "Slow connection",
        comment: "Slow connection problem for contact form issue field."
      )

    public static let contactFormIssueWebsiteProblems =
      NSLocalizedString(
        "vpn.contactFormIssueWebsiteProblems",
        tableName: "BraveShared",
        bundle: .module,
        value: "Website doesn't work",
        comment: "Website problem for contact form issue field."
      )

    public static let contactFormIssueConnectionReliability =
      NSLocalizedString(
        "vpn.contactFormIssueConnectionReliability",
        tableName: "BraveShared",
        bundle: .module,
        value: "Connection reliability problem",
        comment: "Connection problems for contact form issue field."
      )

    public static let contactFormIssueOther =
      NSLocalizedString(
        "vpn.contactFormIssueOther",
        tableName: "BraveShared",
        bundle: .module,
        value: "Other",
        comment: "Other problem for contact form issue field."
      )

    public static let subscriptionStatusExpired =
      NSLocalizedString(
        "vpn.planExpired",
        tableName: "BraveShared",
        bundle: .module,
        value: "Expired",
        comment: "Text to show user when their vpn plan has expired"
      )

    public static let resetVPNErrorTitle =
      NSLocalizedString(
        "vpn.resetVPNErrorTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Reset Failed",
        comment: "Title for error message when vpn configuration reset fails."
      )

    public static let resetVPNErrorBody =
      NSLocalizedString(
        "vpn.resetVPNErrorBody",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Unable to reset VPN configuration. Please try again. If the issue persists, contact support for assistance.",
        comment: "Message to show when vpn configuration reset fails."
      )

    public static let resetVPNErrorButtonActionTitle =
      NSLocalizedString(
        "vpn.resetVPNErrorButtonActionTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Try Again",
        comment: "Title of button to try again when vpn configuration reset fails."
      )

    public static let resetVPNSuccessTitle =
      NSLocalizedString(
        "vpn.resetVPNSuccessTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Reset Successful",
        comment: "Title for success message when vpn configuration reset succeeds."
      )

    public static let resetVPNSuccessBody =
      NSLocalizedString(
        "vpn.resetVPNSuccessBody",
        tableName: "BraveShared",
        bundle: .module,
        value: "VPN configuration has been reset successfully.",
        comment: "Message to show when vpn configuration reset succeeds."
      )

    public static let contactFormDoNotEditText =
      NSLocalizedString(
        "vpn.contactFormDoNotEditText",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Brave doesn’t track you or know how you use our app, so we don’t know how you've set up VPN. Please share info about the issue you're experiencing and we'll do our best to resolve it as soon as we can.",
        comment: "Text to tell user to not modify support info below email's body."
      )

    public static let contactFormTitle =
      NSLocalizedString(
        "vpn.contactFormTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Brave Firewall + VPN Issue",
        comment: "Title for contact form email."
      )

    public static let iapDisclaimer =
      NSLocalizedString(
        "vpn.iapDisclaimer",
        tableName: "BraveShared",
        bundle: .module,
        value: "All subscriptions are auto-renewed but can be cancelled before renewal.",
        comment: "Disclaimer about in app subscription"
      )

    public static let installSuccessPopup =
      NSLocalizedString(
        "vpn.installSuccessPopup",
        tableName: "BraveShared",
        bundle: .module,
        value: "VPN is now enabled",
        comment: "Popup that shows after user installs the vpn for the first time."
      )

    public static let vpnBackgroundNotificationTitle =
      NSLocalizedString(
        "vpn.vpnBackgroundNotificationTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Brave Firewall + VPN is ON",
        comment: "Notification title to tell user that the vpn is turned on even in background"
      )

    public static let vpnBackgroundNotificationBody =
      NSLocalizedString(
        "vpn.vpnBackgroundNotificationBody",
        tableName: "BraveShared",
        bundle: .module,
        value: "Even in the background, Brave will continue to protect you.",
        comment: "Notification title to tell user that the vpn is turned on even in background"
      )

    public static let vpnIAPBoilerPlate =
      NSLocalizedString(
        "vpn.vpnIAPBoilerPlate",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Subscriptions will be charged via your iTunes account.\n\nAny unused portion of the free trial, if offered, is forfeited when you buy a subscription.\n\nYour subscription will renew automatically unless it is cancelled at least 24 hours before the end of the current period.\n\nYou can manage your subscriptions in Settings.\n\nBy using Brave, you agree to the Terms of Use and Privacy Policy.",
        comment: "Disclaimer for user purchasing the VPN plan."
      )

    public static let regionPickerTitle =
      NSLocalizedString(
        "vpn.regionPickerTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Server Region",
        comment: "Title for vpn region selector screen"
      )

    public static let regionPickerAutomaticDescription =
      NSLocalizedString(
        "vpn.regionPickerAutomaticDescription",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "A server region most proximate to you will be automatically selected, based on your system timezone. This is recommended in order to ensure fast internet speeds.",
        comment: "Description of what automatic server selection does."
      )

    public static let regionPickerErrorTitle =
      NSLocalizedString(
        "vpn.regionPickerErrorTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Server Error",
        comment: "Title for error when we fail to switch vpn server for the user"
      )

    public static let regionPickerErrorMessage =
      NSLocalizedString(
        "vpn.regionPickerErrorMessage",
        tableName: "BraveShared",
        bundle: .module,
        value: "Failed to switch servers, please try again later.",
        comment: "Message for error when we fail to switch vpn server for the user"
      )

    public static let protocolPickerTitle =
      NSLocalizedString(
        "vpn.protocolPickerTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Transport Protocol",
        comment: "Title for vpn tunnel protocol screen"
      )

    public static let protocolPickerDescription =
      NSLocalizedString(
        "vpn.protocolPickerDescription",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Please select your preferred transport protocol. Once switched your existing VPN credentials will be cleared and you will be reconnected if a VPN connection is currently established.",
        comment: "Description of vpn tunnel protocol"
      )

    public static let regionSwitchSuccessPopupText =
      NSLocalizedString(
        "vpn.regionSwitchSuccessPopupText",
        tableName: "BraveShared",
        bundle: .module,
        value: "VPN region changed.",
        comment: "Message that we show after successfully changing vpn region."
      )

    public static let protocolPickerErrorTitle =
      NSLocalizedString(
        "vpn.protocolPickerErrorTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Server Error",
        comment: "Title for error when we fail to switch tunnel protocol for the user"
      )

    public static let protocolPickerErrorMessage =
      NSLocalizedString(
        "vpn.protocolPickerErrorMessage",
        tableName: "BraveShared",
        bundle: .module,
        value: "Failed to switch tunnel protocol, please try again later.",
        comment: "Message for error when we fail to switch tunnel protocol for the user"
      )

    public static let protocolSwitchSuccessPopupText =
      NSLocalizedString(
        "vpn.protocolSwitchSuccessPopupText",
        tableName: "BraveShared",
        bundle: .module,
        value: "VPN Tunnel Protocol changed.",
        comment: "Message that we show after successfully changing tunnel protocol."
      )

    public static let settingsFailedToFetchServerList =
      NSLocalizedString(
        "vpn.settingsFailedToFetchServerList",
        tableName: "BraveShared",
        bundle: .module,
        value: "Failed to retrieve server list, please try again later.",
        comment: "Error message shown if we failed to retrieve vpn server list."
      )

    public static let contactFormEmailNotConfiguredBody =
      NSLocalizedString(
        "vpn.contactFormEmailNotConfiguredBody",
        tableName: "BraveShared",
        bundle: .module,
        value: "Can't send email. Please check your email configuration.",
        comment: "Button name to send contact form."
      )

    public static let vpnActionUpdatePaymentMethodSettingsText =
      NSLocalizedString(
        "vpn.vpnActionUpdatePaymentMethodSettingsText",
        tableName: "BraveShared",
        bundle: .module,
        value: "Update Payment Method",
        comment:
          "Text for necesseray required action of VPN subscription needs a method of payment update"
      )

    public static let vpnCityRegionOptimalTitle =
      NSLocalizedString(
        "vpn.vpnCityRegionOptimalTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Optimal",
        comment:
          "Text for title of list item which chooses the optimal server for the country region"
      )

    public static let vpnCityRegionOptimalDescription =
      NSLocalizedString(
        "vpn.vpnCityRegionOptimalDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Use the best server available",
        comment:
          "Text for description of list item which chooses the optimal server for the country region"
      )

    public static let serverRegionAutoSelectDescription =
      NSLocalizedString(
        "vpn.serverRegionAutoSelectDescription",
        tableName: "BraveShared",
        bundle: .module,
        value:
          "Auto-select the VPN server region closest to you based on your timezone. This option is recommended to maximize Internet speeds.",
        comment:
          "Text for description of toggle that auto selcets the server according to time zone."
      )

    public static let serverCountTitle =
      NSLocalizedString(
        "vpn.serverTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "%ld server",
        comment: "Text indicating how many server is used for that region used as Ex: '1 server'"
      )

    public static let multipleServerCountTitle =
      NSLocalizedString(
        "vpn.multipleServerCountTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "%ld servers",
        comment: "Text indicating how many server is used for that region used as Ex: '2 servers'"
      )

    public static let connectedRegionDescription =
      NSLocalizedString(
        "vpn.connectedRegionDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "Connected",
        comment: "Text showing the vpn region is connected"
      )

    public static let automaticServerSelectionToggleTitle =
      NSLocalizedString(
        "vpn.automaticServerSelectionToggleTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "Automatic",
        comment:
          "Text for toggle which indicating the toggle action is for selecting region in automatic"
      )

    public static let availableServerTitle =
      NSLocalizedString(
        "vpn.availableServerTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "AVAILABLE SERVERS",
        comment: "Title on top the list of available servers"
      )

    public static let serverNameTitle =
      NSLocalizedString(
        "vpn.serverNameTitle",
        tableName: "BraveShared",
        bundle: .module,
        value: "%@ Server",
        comment: "Title showing which country serves list showing. Ex: 'Brazil Server"
      )

    public static let vpnRegionChangedTitle =
      NSLocalizedString(
        "vpn.protocolPickerDescription",
        tableName: "BraveShared",
        bundle: .module,
        value: "VPN Region Changed",
        comment: "The alert title showing vpn region is changed"
      )
  }
}
