/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import BraveShared

enum TabBarVisibility: Int {
    case never
    case always
    case landscapeOnly
    
    // TODO: Remove when we can use Swift 4.2/`CaseIterable`
    static let allCases: [TabBarVisibility] = [.never, .always, .landscapeOnly]
}

enum PasswordManagerShortcutBehavior: Int {
    case showPicker
    case onePassword
    case lastPass
    case bitwarden
    case trueKey
    
    // TODO: Remove when we can use Swift 4.2/`CaseIterable`
    static let allCases: [PasswordManagerShortcutBehavior] = [.showPicker, .onePassword, .lastPass, .bitwarden, .trueKey]
}

// MARK: - Other Preferences
extension Preferences {
    final class Popups {
        /// Whether or not the user has seen the DuckDuckGo popup at least once
        ///
        /// Defaults to false, meaning the user has not been given the choice yet
        static let duckDuckGoPrivateSearch = Option<Bool>(key: "popups.ddg-private-search", default: false)
        
        /// Whether or not the user has seen the Browser Lock/PIN popup at least once
        static let browserLock = Option<Bool>(key: "popups.browser-lock", default: false)
    }
    final class AppState {
        /// A flag for determining if the app exited with user interaction in the previous session
        ///
        /// Value should only be checked on launch
        static let backgroundedCleanly = Option<Bool>(key: "appstate.backgrounded-cleanly", default: true)
    }
}

// MARK: - User Preferences
extension Preferences {
    final class General {
        /// Whether this is the first time user has ever launched Brave after intalling. *Should never be set to `true` manually!*
        static let isFirstLaunch = Option<Bool>(key: "general.first-launch", default: true)
        /// Whether or not to save logins in Brave
        static let saveLogins = Option<Bool>(key: "general.save-logins", default: true)
        /// Whether or not to block popups from websites automaticaly
        static let blockPopups = Option<Bool>(key: "general.block-popups", default: true)
        /// Controls how the tab bar should be shown (or not shown)
        static let tabBarVisibility = Option<Int>(key: "general.tab-bar-visiblity", default: TabBarVisibility.always.rawValue)
        /// The behavior when the password manager button is tapped
        static let passwordManagerShortcutBehavior = Option<Int>(key: "general.pm-button-behavior", default: PasswordManagerShortcutBehavior.showPicker.rawValue)
    }
    final class Search {
        /// Whether or not to show suggestions while the user types
        static let showSuggestions = Option<Bool>(key: "search.show-suggestions", default: false)
        /// A list of disabled search engines
        static let disabledEngines = Option<[String]>(key: "search.disabled-engines", default: [])
        /// A list of ordered search engines or nil if they have not been set up yet
        static let orderedEngines = Option<[String]?>(key: "search.ordered-engines", default: nil)
    }
    final class Privacy {
        /// Forces all private tabs
        static let privateBrowsingOnly = Option<Bool>(key: "privacy.private-only", default: false)
        /// The users preference on how to store Cookies
        static let cookieAcceptPolicy = Option<UInt>(key: "privacy.cookie-accept", default: HTTPCookie.AcceptPolicy.onlyFromMainDocumentDomain.rawValue)
        /// The toggles states for clear private data screen
        static let clearPrivateDataToggles = Option<[Bool]>(key: "privacy.clear-data-toggles", default: [])
    }
    final class Shields {
        /// Shields will block ads and tracking if enabled
        static let blockAdsAndTracking = Option<Bool>(key: "shields.block-ads-and-tracking", default: true)
        /// Websites will be upgraded to HTTPS if a loaded page attempts to use HTTP
        static let httpsEverywhere = Option<Bool>(key: "shields.https-everywhere", default: true)
        /// Shields will block websites related to potential phishing and malware
        static let blockPhishingAndMalware = Option<Bool>(key: "shields.block-phishing-and-malware", default: true)
        /// Disables JavaScript execution in the browser
        static let blockScripts = Option<Bool>(key: "shields.block-scripts", default: false)
        /// Enforces fingerprinting protection on the users session
        static let fingerprintingProtection = Option<Bool>(key: "shields.fingerprinting-protection", default: false)
        /// Disables image loading in the browser
        static let blockImages = Option<Bool>(key: "shields.block-images", default: false)
        ///
        static let useRegionAdBlock = Option<Bool>(key: "shields.regional-adblock", default: false)
    }
}

