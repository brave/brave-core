/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import BraveShared

enum TabBarVisibility: Int, CaseIterable {
    case never
    case always
    case landscapeOnly
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
        /// Defines the user's normal browsing theme
        /// `system`, follows the current OS display mode
        static let themeNormalMode = Option<String>(key: "general.normal-mode-theme", default: Theme.DefaultTheme.system.rawValue)
        static let themePrivateMode = Option<String>(key: "general.private-mode-theme", default: Theme.DefaultTheme.private.rawValue)
        /// Specifies whether the bookmark button is present on toolbar
        static let showBookmarkToolbarShortcut = Option<Bool>(key: "general.show-bookmark-toolbar-shortcut", default: UIDevice.isIpad)
        /// Sets Desktop UA for iPad by default (iOS 13+ & iPad only).
        /// Do not read it directly, prefer to use `UserAgent.shouldUseDesktopMode` instead.
        static let alwaysRequestDesktopSite = Option<Bool>(key: "general.always-request-desktop-site", default: UIDevice.isIpad)
        /// Controls whether or not media auto-plays
        static let mediaAutoPlays = Option<Bool>(key: "general.media-auto-plays", default: false)
        
        /// Whether or not to show the clipboard bar when the user has a URL in their pasteboard on launch
        ///
        /// Currently unused.
        static let showClipboardBar = Option<Bool>(key: "general.show-clipboard-bar", default: false)
        /// Whether or not new user onboarding has completed.
        /// User skipping(tapping on skip) onboarding does NOT count as completed.
        /// If user kills the app before completing onboarding, it should be treated as unfinished.
        static let basicOnboardingCompleted = Option<Int>(key: "general.basic-onboarding-completed",
                                                          default: OnboardingState.undetermined.rawValue)
        
        /// The progress the user has made with onboarding
        static let basicOnboardingProgress = Option<Int>(key: "general.basic-onboarding-progress", default: OnboardingProgress.none.rawValue)
        /// Whether or not link preview upon long press action should be shown.
        static let enableLinkPreview = Option<Bool>(key: "general.night-mode", default: true)
        
        static let defaultBrowserCalloutDismissed =
            Option<Bool>(key: "general.default-browser-callout-dismissed", default: false)
    }
    final class Search {
        /// Whether or not to show suggestions while the user types
        static let showSuggestions = Option<Bool>(key: "search.show-suggestions", default: false)
        /// If the user should see the show suggetsions opt-in
        static let shouldShowSuggestionsOptIn = Option<Bool>(key: "search.show-suggestions-opt-in", default: true)
        /// A list of disabled search engines
        static let disabledEngines = Option<[String]?>(key: "search.disabled-engines", default: nil)
        /// A list of ordered search engines or nil if they have not been set up yet
        static let orderedEngines = Option<[String]?>(key: "search.ordered-engines", default: nil)
        /// The default selected search engine in regular mode
        static let defaultEngineName = Option<String?>(key: "search.default.name", default: nil)
        /// The default selected search engine in private mode
        static let defaultPrivateEngineName = Option<String?>(key: "search.defaultprivate.name", default: nil)
    }
    final class Privacy {
        /// Forces all private tabs
        static let privateBrowsingOnly = Option<Bool>(key: "privacy.private-only", default: false)
        /// Blocks all cookies and access to local storage
        static let blockAllCookies = Option<Bool>(key: "privacy.block-all-cookies", default: false)
        /// The toggles states for clear private data screen
        static let clearPrivateDataToggles = Option<[Bool]>(key: "privacy.clear-data-toggles", default: [])
    }
    final class NewTabPage {
        /// Whether bookmark image are enabled / shown
        static let backgroundImages = Option<Bool>(key: "newtabpage.background-images", default: true)
        /// Whether sponsored images are included into the background image rotation
        static let backgroundSponsoredImages = Option<Bool>(key: "newtabpage.background-sponsored-images", default: true)
        
        /// At least one notification must show before we lock showing subsequent notifications.
        static let atleastOneNTPNotificationWasShowed = Option<Bool>(key: "newtabpage.one-notificaiton-showed",
                                                                     default: false)
        
        /// Whether the callout to use branded image was shown.
        static let brandedImageShowed = Option<Bool>(key: "newtabpage.branded-image-callout-showed",
                                                     default: false)
        
        /// When true, a notification on new tab page will be shown that an ad grant can be claimed(if rewards and grant are available).
        /// This value is reseted on each app launch,
        /// The goal is to show the claim grant notification only once per app session if still available.
        static let attemptToShowClaimRewardsNotification =
            Option<Bool>(key: "newtabpage.show-grant-notification", default: true)
        
        /// Whether preloaded favorites have been initialized. Uses custom favorites in case of super referral or default ones instead.
        static let preloadedFavoritiesInitialized =
            Option<Bool>(key: "newtabpage.favorites-initialized", default: false)
        
        /// When super referrer fails to download and user hasn't changed their default favorites we might want to try to replace them
        /// with the ones provided super referrer once available.This should be done only once.
        static let initialFavoritesHaveBeenReplaced =
            Option<Bool>(key: "newtabpage.initial-favorites-replaced", default: false)
        
        /// Custom theme used in app. Nil if default theme is used.
        static let selectedCustomTheme =
            Option<String?>(key: "newtabpage.selected-custom-theme", default: nil)
        
        /// List of currently installed themes on the device.
        static let installedCustomThemes =
            Option<[String]>(key: "newtabpage.installed-custom-themes", default: [])
        
        /// Tells the app whether we should try to fetch super referrer assets again in case of network error.
        static let superReferrerThemeRetryDeadline =
            Option<Date?>(key: "newtabpage.superreferrer-retry-deadline", default: nil)
    }
    
    final class VPN {
        static let popupShowed = Option<Bool>(key: "vpn.popup-showed", default: false)
        static let appLaunchCountForVPNPopup = Option<Int>(key: "vpn.popup-launch-count", default: 0)
        /// We get it from Guardian's servers.
        static let lastPurchaseProductId = Option<String?>(key: "vpn.last-purchase-id", default: nil)
        /// When the current subscription plan expires. It is nil if the user has not bought any vpn plan yet.
        /// In case of receipt expiration this date might be set to some old date(like year 1970)
        /// to make sure vpn expiration logic will be called.
        static let expirationDate = Option<Date?>(key: "vpn.expiration-date", default: nil)
        /// Whether free trial for the vpn expired for the user.
        static let freeTrialUsed = Option<Bool>(key: "vpn.free-trial-used", default: false)
        /// First time after user background the app after after installing vpn, we show a notification to say that the vpn
        /// also works in background.
        static let vpnWorksInBackgroundNotificationShowed =
            Option<Bool>(key: "vpn.vpn-bg-notification-showed", default: false)
        static let vpnSettingHeaderWasDismissed =
            Option<Bool>(key: "vpn.vpn-header-dismissed", default: false)
    }
    
    final class Debug {
        /// When general blocklists were last time updated on the device.
        static let lastGeneralAdblockUpdate = Option<Date?>(key: "last-general-adblock-update", default: nil)
        /// When regional blocklists were last time updated on the device.
        static let lastRegionalAdblockUpdate = Option<Date?>(key: "last-regional-adblock-update", default: nil)
    }
}

