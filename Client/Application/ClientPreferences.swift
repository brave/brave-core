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
        
        /// Whether or not a user has enabled Night Mode.
        ///
        /// Currently unused
        static let nightMode = Option<Bool>(key: "general.night-mode", default: false)
        /// Whether or not to show the clipboard bar when the user has a URL in their pasteboard on launch
        ///
        /// Currently unused.
        static let showClipboardBar = Option<Bool>(key: "general.show-clipboard-bar", default: false)
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
}

