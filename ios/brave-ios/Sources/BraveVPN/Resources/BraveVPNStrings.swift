// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
@_exported import Strings

extension Strings {
  struct VPN {
    public static let freeTrialDetail =
      NSLocalizedString(
        "vpn.freeTrialDetail",
        bundle: .module,
        value: "All plans include a %@!",
        comment:
          "Used in context: All plans include a 'free 7-day trial'! where variable part will be indicating what kind of trial it will include"
      )

    public static let freeTrialPeriod =
      NSLocalizedString(
        "vpn.freeTrialPeriod",
        bundle: .module,
        value: "free 7-day trial",
        comment:
          "Used in context: All plans include a 'free 7-day trial'! - this will be the disclamier for the trial showing it is free and 7 days long"
      )

    public static let freeTrialPeriodAction =
      NSLocalizedString(
        "vpn.freeTrialPeriodAction",
        bundle: .module,
        value: "try 7 days free",
        comment: "The button text that starts the trial action"
      )

    public static let activateSubscriptionAction =
      NSLocalizedString(
        "vpn.activateSubscriptionAction",
        bundle: .module,
        value: "activate",
        comment: "The button text that starts the subscription action"
      )

    public static let restorePurchases =
      NSLocalizedString(
        "vpn.restorePurchases",
        bundle: .module,
        value: "Restore",
        comment: ""
      )

    public static let monthlySubTitle =
      NSLocalizedString(
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

    public static let yearlySubTitle =
      NSLocalizedString(
        "vpn.yearlySubTitle",
        bundle: .module,
        value: "One year",
        comment: "One year lenght vpn subcription"
      )

    public static let yearlySubDetail =
      NSLocalizedString(
        "vpn.yearlySubDetail",
        bundle: .module,
        value: "Renew annually save %@",
        comment:
          "Used in context: 'yearly subscription, renew annually (to) save 16%'. The placeholder is for percent value"
      )

    public static let yearlySubDisclaimer =
      NSLocalizedString(
        "vpn.yearlySubDisclaimer",
        bundle: .module,
        value: "Best value",
        comment:
          "It's like when there's few subscription plans, and one plan has the best value to price ratio, so this label says next to that plan: '(plan) - Best value'"
      )

    // MARK: Checkboxes

    public static let checkboxProtectYourDevices =
      NSLocalizedString(
        "vpn.checkboxProtectYourDevices",
        bundle: .module,
        value: "Protect every app & your whole device",
        comment: "Text for a checkbox to present the user benefits for using Brave VPN"
      )

    public static let checkboxSaferWifi =
      NSLocalizedString(
        "vpn.checkboxSaferWifi",
        bundle: .module,
        value: "Safer for home or public Wi-Fi",
        comment: "Text for a checkbox to present the user benefits for using Brave VPN"
      )

    public static let checkboxSpeedFast =
      NSLocalizedString(
        "vpn.checkboxSpeedFast",
        bundle: .module,
        value: "Lightning-fast, up to 100 Mbps",
        comment: "Text for a checkbox to present the user benefits for using Brave VPN"
      )

    public static let checkboxGeoLocation =
      NSLocalizedString(
        "vpn.checkboxGeoLocation",
        bundle: .module,
        value: "Choose your geo/country location",
        comment: "Text for a checkbox to present the user benefits for using Brave VPN"
      )

    public static let checkboxNoIPLog =
      NSLocalizedString(
        "vpn.checkboxNoIPLog",
        bundle: .module,
        value: "Brave never logs your activity",
        comment: "Text for a checkbox to present the user benefits for using Brave VPN"
      )

    public static let checkboxDevicesProtect =
      NSLocalizedString(
        "vpn.checkboxDevicesProtect",
        bundle: .module,
        value: "Protect 5 devices on 1 subscription",
        comment: "Text for a checkbox to present the user benefits for using Brave VPN"
      )

    public static let installTitle =
      NSLocalizedString(
        "vpn.installTitle",
        bundle: .module,
        value: "Install VPN",
        comment: "Title for screen to install the VPN."
      )

    public static let installProfileTitle =
      NSLocalizedString(
        "vpn.installProfileTitle",
        bundle: .module,
        value: "Brave will now install a VPN profile.",
        comment: ""
      )

    public static let installProfileBody =
      NSLocalizedString(
        "vpn.installProfileBody",
        bundle: .module,
        value:
          "This profile allows the VPN to automatically connect and secure traffic across your device all the time. This VPN connection will be encrypted and routed through Brave's intelligent firewall to block potentially harmful and invasive connections.",
        comment: "Text explaining how the VPN works."
      )

    public static let installProfileButtonText =
      NSLocalizedString(
        "vpn.installProfileButtonText",
        bundle: .module,
        value: "Install VPN Profile",
        comment: "Text for 'install vpn profile' button"
      )

    public static let settingsSubscriptionSection =
      NSLocalizedString(
        "vpn.settingsSubscriptionSection",
        bundle: .module,
        value: "Subscription",
        comment: "Header title for vpn settings subscription section."
      )

    public static let settingsServerSection =
      NSLocalizedString(
        "vpn.settingsServerSection",
        bundle: .module,
        value: "Server",
        comment: "Header title for vpn settings server section."
      )

    public static let settingsSubscriptionStatus =
      NSLocalizedString(
        "vpn.settingsSubscriptionStatus",
        bundle: .module,
        value: "Status",
        comment: "Table cell title for status of current VPN subscription."
      )

    public static let settingsSubscriptionExpiration =
      NSLocalizedString(
        "vpn.settingsSubscriptionExpiration",
        bundle: .module,
        value: "Expires",
        comment: "Table cell title for cell that shows when the VPN subscription expires."
      )

    public static let settingsManageSubscription =
      NSLocalizedString(
        "vpn.settingsManageSubscription",
        bundle: .module,
        value: "Manage Subscription",
        comment: "Button to manage your VPN subscription"
      )

    public static let settingsRedeemOfferCode =
      NSLocalizedString(
        "vpn.settingsRedeemOfferCode",
        bundle: .module,
        value: "Redeem Offer Code",
        comment: "Button to redeem offer code subscription"
      )

    public static let settingsLinkReceipt =
      NSLocalizedString(
        "vpn.settingsLinkReceipt",
        bundle: .module,
        value: "Link purchase to your Brave account",
        comment: "Button to link your VPN receipt to other devices."
      )

    public static let settingsLinkReceiptFooter =
      NSLocalizedString(
        "vpn.settingsLinkReceiptFooter",
        bundle: .module,
        value:
          "Link your App Store purchase to your Brave account to use Brave VPN on other devices.",
        comment: "Footer text to link your VPN receipt to other devices."
      )

    public static let settingsViewReceipt =
      NSLocalizedString(
        "vpn.settingsViewReceipt",
        bundle: .module,
        value: "View AppStore Receipt",
        comment: "Button to allow the user to view the app-store receipt."
      )

    public static let settingsServerHost =
      NSLocalizedString(
        "vpn.settingsServerHost",
        bundle: .module,
        value: "Host",
        comment: "Table cell title for vpn's server host"
      )

    public static let settingsServerLocation =
      NSLocalizedString(
        "vpn.settingsServerLocation",
        bundle: .module,
        value: "Location",
        comment: "Table cell title for vpn's server location and which open opens location select"
      )

    public static let settingsResetConfiguration =
      NSLocalizedString(
        "vpn.settingsResetConfiguration",
        bundle: .module,
        value: "Reset Configuration",
        comment: "Button to reset VPN configuration"
      )

    public static let settingsTransportProtocol =
      NSLocalizedString(
        "vpn.settingsTransportProtocol",
        bundle: .module,
        value: "Transport Protocol",
        comment:
          "Table cell title for vpn's transport protocol and which open opens protocol select"
      )

    public static let settingsContactSupport =
      NSLocalizedString(
        "vpn.settingsContactSupport",
        bundle: .module,
        value: "Contact Technical Support",
        comment: "Button to contact tech support"
      )

    public static let settingsFAQ =
      NSLocalizedString(
        "vpn.settingsFAQ",
        bundle: .module,
        value: "VPN Support",
        comment: "Button for FAQ"
      )

    public static let enableButton =
      NSLocalizedString(
        "vpn.enableButton",
        bundle: .module,
        value: "Enable",
        comment: "Button text to enable Brave VPN"
      )

    public static let buyButton =
      NSLocalizedString(
        "vpn.buyButton",
        bundle: .module,
        value: "Buy",
        comment: "Button text to buy Brave VPN"
      )

    public static let tryForFreeButton =
      NSLocalizedString(
        "vpn.learnMore",
        bundle: .module,
        value: "Try for FREE",
        comment: "Button text to try free Brave VPN"
      )

    public static let settingHeaderBody =
      NSLocalizedString(
        "vpn.settingHeaderBody",
        bundle: .module,
        value:
          "Upgrade to a VPN to protect your connection and block invasive trackers everywhere.",
        comment: "VPN Banner Description"
      )

    public static let vpnConfigGenericErrorTitle =
      NSLocalizedString(
        "vpn.vpnConfigGenericErrorTitle",
        bundle: .module,
        value: "Error",
        comment: "Title for an alert when the VPN can't be configured"
      )

    public static let vpnConfigGenericErrorBody =
      NSLocalizedString(
        "vpn.vpnConfigGenericErrorBody",
        bundle: .module,
        value:
          "There was a problem initializing the VPN. Please try again or try resetting configuration in the VPN settings page.",
        comment: "Message for an alert when the VPN can't be configured."
      )

    public static let vpnConfigPermissionDeniedErrorTitle =
      NSLocalizedString(
        "vpn.vpnConfigPermissionDeniedErrorTitle",
        bundle: .module,
        value: "Permission denied",
        comment: "Title for an alert when the user didn't allow to install VPN profile"
      )

    public static let vpnRedeemCodeButtonActionTitle =
      NSLocalizedString(
        "vpn.vpnRedeemCodeButtonActionTitle",
        bundle: .module,
        value: "Redeem Code",
        comment: "Title for a button for enabling the Redeem Code flow"
      )

    public static let vpnConfigPermissionDeniedErrorBody =
      NSLocalizedString(
        "vpn.vpnConfigPermissionDeniedErrorBody",
        bundle: .module,
        value:
          "The Brave Firewall + VPN requires a VPN profile to be installed on your device to work. ",
        comment: "Title for an alert when the user didn't allow to install VPN profile"
      )

    public static let vpnSettingsMonthlySubName =
      NSLocalizedString(
        "vpn.vpnSettingsMonthlySubName",
        bundle: .module,
        value: "Monthly subscription",
        comment: "Name of monthly subscription in VPN Settings"
      )

    public static let vpnSettingsYearlySubName =
      NSLocalizedString(
        "vpn.vpnSettingsYearlySubName",
        bundle: .module,
        value: "Yearly subscription",
        comment: "Name of annual subscription in VPN Settings"
      )

    public static let vpnErrorPurchaseFailedTitle =
      NSLocalizedString(
        "vpn.vpnErrorPurchaseFailedTitle",
        bundle: .module,
        value: "Error",
        comment: "Title for error when VPN could not be purchased."
      )

    public static let vpnErrorPurchaseFailedBody =
      NSLocalizedString(
        "vpn.vpnErrorPurchaseFailedBody",
        bundle: .module,
        value:
          "Unable to complete purchase. Please try again, or check your payment details on Apple and try again.",
        comment: "Message for error when VPN could not be purchased."
      )

    public static let vpnErrorOfferCodeFailedBody =
      NSLocalizedString(
        "vpn.vpnErrorOfferCodeFailedBody",
        bundle: .module,
        value:
          "Unable to redeem offer code. Please try again, or check your offer code details and try again.",
        comment: "Message for error when VPN offer code could not be redeemed."
      )

    public static let vpnResetAlertTitle =
      NSLocalizedString(
        "vpn.vpnResetAlertTitle",
        bundle: .module,
        value: "Reset configuration",
        comment: "Title for alert to reset vpn configuration"
      )

    public static let vpnResetAlertBody =
      NSLocalizedString(
        "vpn.vpnResetAlertBody",
        bundle: .module,
        value:
          "This will reset your Brave Firewall + VPN configuration and fix any errors. This process may take a minute.",
        comment: "Message for alert to reset vpn configuration"
      )

    public static let vpnResetButton =
      NSLocalizedString(
        "vpn.vpnResetButton",
        bundle: .module,
        value: "Reset",
        comment: "Button name to reset vpn configuration"
      )

    public static let contactFormHostname =
      NSLocalizedString(
        "vpn.contactFormHostname",
        bundle: .module,
        value: "VPN Hostname",
        comment: "VPN Hostname field for customer support contact form."
      )

    public static let contactFormSubscriptionType =
      NSLocalizedString(
        "vpn.contactFormSubscriptionType",
        bundle: .module,
        value: "Subscription Type",
        comment: "Subscription Type field for customer support contact form."
      )

    public static let contactFormAppStoreReceipt =
      NSLocalizedString(
        "vpn.contactFormAppStoreReceipt",
        bundle: .module,
        value: "App Store Receipt",
        comment: "App Store Receipt field for customer support contact form."
      )

    public static let contactFormAppVersion =
      NSLocalizedString(
        "vpn.contactFormAppVersion",
        bundle: .module,
        value: "App Version",
        comment: "App Version field for customer support contact form."
      )

    public static let contactFormTimezone =
      NSLocalizedString(
        "vpn.contactFormTimezone",
        bundle: .module,
        value: "iOS Timezone",
        comment: "iOS Timezone field for customer support contact form."
      )

    public static let contactFormPlatform =
      NSLocalizedString(
        "vpn.contactFormPlatform",
        bundle: .module,
        value: "Platform",
        comment: "A contact form field that displays platform type: 'iOS' 'Android' or 'Desktop'"
      )

    public static let contactFormNetworkType =
      NSLocalizedString(
        "vpn.contactFormNetworkType",
        bundle: .module,
        value: "Network Type",
        comment: "Network Type field for customer support contact form."
      )

    public static let contactFormCarrier =
      NSLocalizedString(
        "vpn.contactFormCarrier",
        bundle: .module,
        value: "Cellular Carrier",
        comment: "Cellular Carrier field for customer support contact form."
      )

    public static let contactFormLogs =
      NSLocalizedString(
        "vpn.contactFormLogs",
        bundle: .module,
        value: "Error Logs",
        comment: "VPN logs field for customer support contact form."
      )

    public static let contactFormIssue =
      NSLocalizedString(
        "vpn.contactFormIssue",
        bundle: .module,
        value: "Issue",
        comment: "Specific issue field for customer support contact form."
      )

    public static let contactFormIssueDescription =
      NSLocalizedString(
        "vpn.contactFormIssueDescription",
        bundle: .module,
        value: "Please choose the category that describes the issue.",
        comment: "Description used for specific issue field for customer support contact form."
      )

    public static let contactFormFooterSharedWithGuardian =
      NSLocalizedString(
        "vpn.contactFormFooterSharedWithGuardian",
        bundle: .module,
        value: "Support provided with the help of the Guardian team.",
        comment: "Footer for customer support contact form."
      )

    public static let contactFormFooter =
      NSLocalizedString(
        "vpn.contactFormFooter",
        bundle: .module,
        value:
          "Please select the information you're comfortable sharing with us.\n\nThe more information you initially share with us the easier it will be for our support staff to help you resolve your issue.",
        comment: "Footer for customer support contact form."
      )

    public static let contactFormSendButton =
      NSLocalizedString(
        "vpn.contactFormSendButton",
        bundle: .module,
        value: "Continue to Email",
        comment: "Button name to send contact form."
      )

    public static let contactFormIssueOtherConnectionError =
      NSLocalizedString(
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

    public static let contactFormIssueSlowConnection =
      NSLocalizedString(
        "vpn.contactFormIssueSlowConnection",
        bundle: .module,
        value: "Slow connection",
        comment: "Slow connection problem for contact form issue field."
      )

    public static let contactFormIssueWebsiteProblems =
      NSLocalizedString(
        "vpn.contactFormIssueWebsiteProblems",
        bundle: .module,
        value: "Website doesn't work",
        comment: "Website problem for contact form issue field."
      )

    public static let contactFormIssueConnectionReliability =
      NSLocalizedString(
        "vpn.contactFormIssueConnectionReliability",
        bundle: .module,
        value: "Connection reliability problem",
        comment: "Connection problems for contact form issue field."
      )

    public static let contactFormIssueOther =
      NSLocalizedString(
        "vpn.contactFormIssueOther",
        bundle: .module,
        value: "Other",
        comment: "Other problem for contact form issue field."
      )

    public static let subscriptionStatusExpired =
      NSLocalizedString(
        "vpn.planExpired",
        bundle: .module,
        value: "Expired",
        comment: "Text to show user when their vpn plan has expired"
      )

    public static let resetVPNErrorTitle =
      NSLocalizedString(
        "vpn.resetVPNErrorTitle",
        bundle: .module,
        value: "Reset Failed",
        comment: "Title for error message when vpn configuration reset fails."
      )

    public static let resetVPNErrorBody =
      NSLocalizedString(
        "vpn.resetVPNErrorBody",
        bundle: .module,
        value:
          "Unable to reset VPN configuration. Please try again. If the issue persists, contact support for assistance.",
        comment: "Message to show when vpn configuration reset fails."
      )

    public static let resetVPNErrorButtonActionTitle =
      NSLocalizedString(
        "vpn.resetVPNErrorButtonActionTitle",
        bundle: .module,
        value: "Try Again",
        comment: "Title of button to try again when vpn configuration reset fails."
      )

    public static let resetVPNSuccessTitle =
      NSLocalizedString(
        "vpn.resetVPNSuccessTitle",
        bundle: .module,
        value: "Reset Successful",
        comment: "Title for success message when vpn configuration reset succeeds."
      )

    public static let resetVPNSuccessBody =
      NSLocalizedString(
        "vpn.resetVPNSuccessBody",
        bundle: .module,
        value: "VPN configuration has been reset successfully.",
        comment: "Message to show when vpn configuration reset succeeds."
      )

    public static let contactFormDoNotEditText =
      NSLocalizedString(
        "vpn.contactFormDoNotEditText",
        bundle: .module,
        value:
          "Brave doesn’t track you or know how you use our app, so we don’t know how you've set up VPN. Please share info about the issue you're experiencing and we'll do our best to resolve it as soon as we can.",
        comment: "Text to tell user to not modify support info below email's body."
      )

    public static let contactFormTitle =
      NSLocalizedString(
        "vpn.contactFormTitle",
        bundle: .module,
        value: "Brave Firewall + VPN Issue",
        comment: "Title for contact form email."
      )

    public static let iapDisclaimer =
      NSLocalizedString(
        "vpn.iapDisclaimer",
        bundle: .module,
        value: "All subscriptions are auto-renewed but can be cancelled before renewal.",
        comment: "Disclaimer about in app subscription"
      )

    public static let installSuccessPopup =
      NSLocalizedString(
        "vpn.installSuccessPopup",
        bundle: .module,
        value: "VPN is now enabled",
        comment: "Popup that shows after user installs the vpn for the first time."
      )

    public static let vpnBackgroundNotificationTitle =
      NSLocalizedString(
        "vpn.vpnBackgroundNotificationTitle",
        bundle: .module,
        value: "Brave Firewall + VPN is ON",
        comment: "Notification title to tell user that the vpn is turned on even in background"
      )

    public static let vpnBackgroundNotificationBody =
      NSLocalizedString(
        "vpn.vpnBackgroundNotificationBody",
        bundle: .module,
        value: "Even in the background, Brave will continue to protect you.",
        comment: "Notification title to tell user that the vpn is turned on even in background"
      )

    public static let vpnIAPBoilerPlate =
      NSLocalizedString(
        "vpn.vpnIAPBoilerPlate",
        bundle: .module,
        value:
          "Subscriptions will be charged via your iTunes account.\n\nAny unused portion of the free trial, if offered, is forfeited when you buy a subscription.\n\nYour subscription will renew automatically unless it is cancelled at least 24 hours before the end of the current period.\n\nYou can manage your subscriptions in Settings.\n\nBy using Brave, you agree to the Terms of Use and Privacy Policy.",
        comment: "Disclaimer for user purchasing the VPN plan."
      )

    public static let regionPickerTitle =
      NSLocalizedString(
        "vpn.regionPickerTitle",
        bundle: .module,
        value: "Server Region",
        comment: "Title for vpn region selector screen"
      )

    public static let regionPickerAutomaticDescription =
      NSLocalizedString(
        "vpn.regionPickerAutomaticDescription",
        bundle: .module,
        value:
          "A server region most proximate to you will be automatically selected, based on your system timezone. This is recommended in order to ensure fast internet speeds.",
        comment: "Description of what automatic server selection does."
      )

    public static let regionPickerErrorTitle =
      NSLocalizedString(
        "vpn.regionPickerErrorTitle",
        bundle: .module,
        value: "Server Error",
        comment: "Title for error when we fail to switch vpn server for the user"
      )

    public static let regionPickerErrorMessage =
      NSLocalizedString(
        "vpn.regionPickerErrorMessage",
        bundle: .module,
        value: "Failed to switch servers, please try again later.",
        comment: "Message for error when we fail to switch vpn server for the user"
      )

    public static let protocolPickerTitle =
      NSLocalizedString(
        "vpn.protocolPickerTitle",
        bundle: .module,
        value: "Transport Protocol",
        comment: "Title for vpn tunnel protocol screen"
      )

    public static let protocolPickerDescription =
      NSLocalizedString(
        "vpn.protocolPickerDescription",
        bundle: .module,
        value:
          "Please select your preferred transport protocol. Once switched your existing VPN credentials will be cleared and you will be reconnected if a VPN connection is currently established.",
        comment: "Description of vpn tunnel protocol"
      )

    public static let regionSwitchSuccessPopupText =
      NSLocalizedString(
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

    public static let protocolPickerErrorMessage =
      NSLocalizedString(
        "vpn.protocolPickerErrorMessage",
        bundle: .module,
        value: "Failed to switch tunnel protocol, please try again later.",
        comment: "Message for error when we fail to switch tunnel protocol for the user"
      )

    public static let protocolSwitchSuccessPopupText =
      NSLocalizedString(
        "vpn.protocolSwitchSuccessPopupText",
        bundle: .module,
        value: "VPN Tunnel Protocol changed.",
        comment: "Message that we show after successfully changing tunnel protocol."
      )

    public static let settingsFailedToFetchServerList =
      NSLocalizedString(
        "vpn.settingsFailedToFetchServerList",
        bundle: .module,
        value: "Failed to retrieve server list, please try again later.",
        comment: "Error message shown if we failed to retrieve vpn server list."
      )

    public static let contactFormEmailNotConfiguredBody =
      NSLocalizedString(
        "vpn.contactFormEmailNotConfiguredBody",
        bundle: .module,
        value: "Can't send email. Please check your email configuration.",
        comment: "Button name to send contact form."
      )

    public static let vpnActionUpdatePaymentMethodSettingsText =
      NSLocalizedString(
        "vpn.vpnActionUpdatePaymentMethodSettingsText",
        bundle: .module,
        value: "Update Payment Method",
        comment:
          "Text for necesseray required action of VPN subscription needs a method of payment update"
      )

    public static let vpnCityRegionOptimalTitle =
      NSLocalizedString(
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

    public static let serverRegionAutoSelectDescription =
      NSLocalizedString(
        "vpn.serverRegionAutoSelectDescription",
        bundle: .module,
        value:
          "Auto-select the VPN server region closest to you based on your timezone. This option is recommended to maximize Internet speeds.",
        comment:
          "Text for description of toggle that auto selcets the server according to time zone."
      )

    public static let cityCountTitle =
      NSLocalizedString(
        "vpn.cityCountTitle",
        bundle: .module,
        value: "%ld City",
        comment: "Text indicating how many cities is used for that country used as Ex: '1 City'"
      )

    public static let multipleCityCountTitle =
      NSLocalizedString(
        "vpn.multipleCityCountTitle",
        bundle: .module,
        value: "%ld Cities",
        comment: "Text indicating how many cities is used for that region used as Ex: '2 Cities'"
      )

    public static let serverCountTitle =
      NSLocalizedString(
        "vpn.serverTitle",
        bundle: .module,
        value: "%ld Server",
        comment: "Text indicating how many server is used for that region used as Ex: '1 Server'"
      )

    public static let multipleServerCountTitle =
      NSLocalizedString(
        "vpn.multipleServerCountTitle",
        bundle: .module,
        value: "%ld Servers",
        comment: "Text indicating how many server is used for that region used as Ex: '2 Servers'"
      )

    public static let connectedRegionDescription =
      NSLocalizedString(
        "vpn.connectedRegionDescription",
        bundle: .module,
        value: "Connected",
        comment: "Text showing the vpn region is connected"
      )

    public static let automaticServerSelectionToggleTitle =
      NSLocalizedString(
        "vpn.automaticServerSelectionToggleTitle",
        bundle: .module,
        value: "Automatic",
        comment:
          "Text for toggle which indicating the toggle action is for selecting region in automatic"
      )

    public static let availableServerTitle =
      NSLocalizedString(
        "vpn.availableServerTitle",
        bundle: .module,
        value: "AVAILABLE SERVERS",
        comment: "Title on top the list of available servers"
      )

    public static let serverNameTitle =
      NSLocalizedString(
        "vpn.serverNameTitle",
        bundle: .module,
        value: "%@ Server",
        comment: "Title showing which country serves list showing. Ex: 'Brazil Server"
      )

    public static let vpnRegionChangedTitle =
      NSLocalizedString(
        "vpn.vpnRegionChangedTitle",
        bundle: .module,
        value: "VPN Region Changed",
        comment: "The alert title showing vpn region is changed"
      )
  }
}
