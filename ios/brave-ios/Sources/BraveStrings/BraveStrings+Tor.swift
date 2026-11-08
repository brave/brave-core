// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the Mozilla Public
// License, v. 2.0. See https://mozilla.org/MPL/2.0/.
import Foundation

extension Strings {
  public enum Tor {
    public static let connectionStatus = NSLocalizedString(
      "Tor.connectionStatus", bundle: .module, value: "Tor Status",
      comment: "Tor connection status menu: connectionStatus"
    )
    public static let reloadConnection = NSLocalizedString(
      "Tor.reloadConnection", bundle: .module, value: "Reload Tor",
      comment: "Tor connection status menu: reloadConnection"
    )
    public static let disableAutomaticPopup = NSLocalizedString(
      "Tor.disableAutomaticPopup", bundle: .module, value: "Disable automatic popup",
      comment: "Tor connection status menu: disableAutomaticPopup"
    )
    public static let settings = NSLocalizedString(
      "Tor.settings", bundle: .module,
      value: "Tor",
      comment: "Tor settings: settings"
    )
    public static let enabled = NSLocalizedString(
      "Tor.enabled", bundle: .module,
      value: "Private Tabs with Tor",
      comment: "Tor settings: enabled"
    )
    public static let enabledDescription = NSLocalizedString(
      "Tor.enabledDescription", bundle: .module,
      value: "Tor hides your IP address from the sites you visit. Turning this off closes all Tor tabs.",
      comment: "Tor settings: enabledDescription"
    )
    public static let onionOnly = NSLocalizedString(
      "Tor.onionOnly", bundle: .module,
      value: "Only Resolve .onion Addresses in Tor Tabs",
      comment: "Tor settings: onionOnly"
    )
    public static let onionOnlyDescription = NSLocalizedString(
      "Tor.onionOnlyDescription", bundle: .module,
      value: "Block onion requests in regular and private tabs. When off, resolution depends on your network configuration.",
      comment: "Tor settings: onionOnlyDescription"
    )
    public static let bridges = NSLocalizedString(
      "Tor.bridges", bundle: .module,
      value: "Bridges",
      comment: "Tor settings: bridges section heading"
    )
    public static let useBridges = NSLocalizedString(
      "Tor.useBridges", bundle: .module,
      value: "Use Bridges",
      comment: "Tor settings: useBridges"
    )
    public static let bridgesDescription = NSLocalizedString(
      "Tor.bridgesDescription", bundle: .module,
      value: "Bridges help you access the Tor network in places where Tor is blocked. Depending on your location, one bridge may work better than another.",
      comment: "Tor settings: bridgesDescription"
    )
    public static let bridgeSource = NSLocalizedString(
      "Tor.bridgeSource", bundle: .module,
      value: "Bridge Source",
      comment: "Tor settings: bridgeSource"
    )
    public static let builtIn = NSLocalizedString(
      "Tor.builtIn", bundle: .module,
      value: "Select a Built-in Bridge",
      comment: "Tor settings: builtIn"
    )
    public static let builtInType = NSLocalizedString(
      "Tor.builtInType", bundle: .module,
      value: "Built-in Bridge",
      comment: "Tor settings: builtInType"
    )
    public static let requestBridge = NSLocalizedString(
      "Tor.requestBridge", bundle: .module,
      value: "Request a Bridge from torproject.org",
      comment: "Tor settings: requestBridge"
    )
    public static let provideBridge = NSLocalizedString(
      "Tor.provideBridge", bundle: .module,
      value: "Provide a Bridge",
      comment: "Tor settings: provideBridge"
    )
    public static let provideDescription = NSLocalizedString(
      "Tor.provideDescription", bundle: .module,
      value: "Enter bridge information from a trusted source, one bridge per line.",
      comment: "Tor settings: provideDescription"
    )
    public static let requestedBridges = NSLocalizedString(
      "Tor.requestedBridges", bundle: .module,
      value: "Requested Bridges",
      comment: "Tor settings: requestedBridges"
    )
    public static let newBridge = NSLocalizedString(
      "Tor.newBridge", bundle: .module,
      value: "Request a New Bridge…",
      comment: "Tor settings: newBridge"
    )
    public static let applyBridges = NSLocalizedString(
      "Tor.applyBridges", bundle: .module,
      value: "Apply Changes",
      comment: "Tor settings: applyBridges"
    )
    public static let bridgeApplyDescription = NSLocalizedString(
      "Tor.bridgeApplyDescription", bundle: .module,
      value: "Apply changes to reconnect Tor using this bridge configuration.",
      comment: "Tor settings: bridgeApplyDescription"
    )
    public static let bridgeInvalid = NSLocalizedString(
      "Tor.bridgeInvalid", bundle: .module,
      value: "Enter valid bridge lines with numeric IP addresses and ports. Supported transports are obfs4, Snowflake, and meek_lite.",
      comment: "Tor settings: bridgeInvalid"
    )
    public static let bridgeRequestDescription = NSLocalizedString(
      "Tor.bridgeRequestDescription", bundle: .module,
      value: "This contacts Tor Project directly so you can request bridges while Tor is blocked.",
      comment: "Tor settings: bridgeRequestDescription"
    )
    public static let captchaTitle = NSLocalizedString(
      "Tor.captchaTitle", bundle: .module,
      value: "Request Tor Bridges",
      comment: "Tor settings: captchaTitle"
    )
    public static let captchaPrompt = NSLocalizedString(
      "Tor.captchaPrompt", bundle: .module,
      value: "Enter the characters shown in the image.",
      comment: "Tor settings: captchaPrompt"
    )
    public static let captchaSolution = NSLocalizedString(
      "Tor.captchaSolution", bundle: .module,
      value: "CAPTCHA Solution",
      comment: "Tor settings: captchaSolution"
    )
    public static let captchaRefresh = NSLocalizedString(
      "Tor.captchaRefresh", bundle: .module,
      value: "New CAPTCHA",
      comment: "Tor settings: captchaRefresh"
    )
    public static let captchaSubmit = NSLocalizedString(
      "Tor.captchaSubmit", bundle: .module,
      value: "Submit",
      comment: "Tor settings: captchaSubmit"
    )
    public static let captchaError = NSLocalizedString(
      "Tor.captchaError", bundle: .module,
      value: "Tor Project could not complete the request. Try a new CAPTCHA or provide a bridge from a trusted source.",
      comment: "Tor settings: captchaError"
    )
    public static let learnMore = NSLocalizedString(
      "Tor.learnMore", bundle: .module,
      value: "Learn More",
      comment: "Tor settings: learnMore"
    )
    public static let openOnionLocation = NSLocalizedString(
      "Tor.openOnionLocation", bundle: .module,
      value: "Open Onion Site in New Private with Tor Tab",
      comment: "URL bar action for an Onion-Location advertised by the current website"
    )
    public static let openLinkInNewTab = NSLocalizedString(
      "Tor.openLinkInNewTab", bundle: .module, value: "Open in New Private with Tor Tab",
      comment: "Link context menu action that opens the link in a new Tor tab"
    )
    public static let shortcutTitle = NSLocalizedString(
      "Tor.shortcutTitle", bundle: .module, value: "Open Private with Tor Tab",
      comment: "Siri shortcut to open a new private tab using Tor"
    )
    public static let shortcutDescription = NSLocalizedString(
      "Tor.shortcutDescription", bundle: .module,
      value: "Open a new Private with Tor tab in Brave.",
      comment: "Description of the Tor Siri shortcut"
    )
    public static let shortcutPhrase = NSLocalizedString(
      "Tor.shortcutPhrase", bundle: .module, value: "Open Private with Tor",
      comment: "Suggested invocation phrase for the Tor Siri shortcut"
    )
    public static let searchTab = NSLocalizedString(
      "Tor.searchTab", bundle: .module, value: "Private with Tor Tab",
      comment: "Tor tab default search engine setting and picker title"
    )
    public static let keepTabsSeparate = NSLocalizedString(
      "Tor.keepTabsSeparate", bundle: .module,
      value: "Keep Tor tabs separate",
      comment: "Tor connection isolation setting in Private Tabs"
    )
    public static let keepTabsSeparateOn = NSLocalizedString(
      "Tor.keepTabsSeparateOn", bundle: .module,
      value: "On (Recommended)",
      comment: "Recommended per-tab Tor connection isolation option"
    )
    public static let keepTabsSeparateOnDescription = NSLocalizedString(
      "Tor.keepTabsSeparateOnDescription", bundle: .module,
      value: "Each Tor tab keeps cookies and other website data separate and uses its own Tor connection. This helps prevent websites from linking activity across tabs.",
      comment: "Explanation of isolated Tor website data and connections"
    )
    public static let keepTabsSeparateOff = NSLocalizedString(
      "Tor.keepTabsSeparateOff", bundle: .module,
      value: "Off",
      comment: "Shared Tor connections option"
    )
    public static let keepTabsSeparateOffDescription = NSLocalizedString(
      "Tor.keepTabsSeparateOffDescription", bundle: .module,
      value: "Tor tabs share cookies, login sessions, other website data, and Tor connections. Activity may be easier to link across tabs and websites.",
      comment: "Explanation of shared Tor website data and connections"
    )
    public static let keepTabsSeparateFooter = NSLocalizedString(
      "Tor.keepTabsSeparateFooter", bundle: .module,
      value: "Applies to new Tor tabs. Close and reopen existing tabs to change their setting. Shared data is cleared when the last tab using shared data is closed.",
      comment: "When Tor connection isolation changes take effect"
    )
    public static let newTab = NSLocalizedString(
      "Tor.newTab", bundle: .module,
      value: "New Tor tab",
      comment: "Open a private Tor tab"
    )
    public static let reload = NSLocalizedString(
      "Tor.reload", bundle: .module,
      value: "Reload",
      comment: "Reload Tor page"
    )
    public static let title = NSLocalizedString(
      "Tor.title", bundle: .module,
      value: "Private with Tor",
      comment: "Tor tab group title"
    )
    public static let waiting = NSLocalizedString(
      "Tor.waiting", bundle: .module,
      value: "Waiting for a network…",
      comment: "Tor paused"
    )
    public static let connecting = NSLocalizedString(
      "Tor.connecting", bundle: .module,
      value: "Connecting through Tor…",
      comment: "Tor connecting"
    )
    public static let finishingConnection = NSLocalizedString(
      "Tor.finishingConnection", bundle: .module,
      value: "Finishing up…",
      comment: "Tor bootstrap has completed but the first connection is not ready yet"
    )
    public static let reconnecting = NSLocalizedString(
      "Tor.reconnecting", bundle: .module,
      value: "Reconnecting…",
      comment: "Tor is restoring a previously established connection"
    )
    public static let connected = NSLocalizedString(
      "Tor.connected", bundle: .module,
      value: "Connected through Tor",
      comment: "Tor connected"
    )
    public static let unavailable = NSLocalizedString(
      "Tor.unavailable", bundle: .module,
      value: "Tor unavailable. Browsing is paused.",
      comment: "Tor cannot connect"
    )
    public static let utcUnavailable = NSLocalizedString(
      "Tor.utcUnavailable", bundle: .module,
      value: "Tor protections are unavailable on this iOS version. No page was loaded.",
      comment: "Mandatory Tor protection unavailable"
    )
    public static let privacyTitle = NSLocalizedString(
      "Tor.privacyTitle", bundle: .module,
      value: "Browse through the Tor network",
      comment: "Tor empty tab group heading"
    )
    public static let privacyBody = NSLocalizedString(
      "Tor.privacyBody", bundle: .module,
      value: "Tor connects automatically. These tabs keep browsing data separate from other browsing modes and use UTC time. Network changes pause traffic while keeping your pages open.",
      comment: "Tor empty tab group explanation"
    )
    public static let limitations = NSLocalizedString(
      "Tor.limitations", bundle: .module,
      value: "Tor protects these tabs. Downloads, media, translation, and other browser services are unavailable in this build.",
      comment: "Tor integration feature limitations"
    )
  }
}
