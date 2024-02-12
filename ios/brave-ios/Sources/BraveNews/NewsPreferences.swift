// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Preferences

extension Preferences {
  public final class BraveNews {
    public static let isShowingOptIn = Option<Bool>(key: "brave-today.showing-opt-in", default: false)
    public static let userOptedIn = Option<Bool>(key: "brave-today.user-opted-in", default: false)
    public static let isEnabled = Option<Bool>(key: "brave-today.enabled", default: true)
    public static let languageChecked = Option<Bool>(key: "brave-today.language-checked", default: false)
    public static let languageWasUnavailableDuringCheck = Option<Bool?>(key: "brave-today.language-unavailable-when-checked", default: nil)
    public static let debugEnvironment = Option<String?>(key: "brave-today.debug.environment", default: nil)
    /// A list of channels followed for a locale.
    ///
    /// Example: ["en_US": "Top Sources"]
    public static let followedChannels = Option<[String: [String]]>(key: "brave-news.followed-channels", default: [:])
    public static let isNewsRevampSetUpCompleted = Option<Bool>(key: "brave-news.revamp.setup.completed", default: false)
    public static let selectedLocale = Option<String?>(key: "brave-news.selected-locale", default: nil)
  }
}
