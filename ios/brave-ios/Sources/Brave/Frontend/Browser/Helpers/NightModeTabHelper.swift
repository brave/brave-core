// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Preferences
import Web

extension TabDataValues {
  private struct NightModeTabHelperKey: TabDataKey {
    static var defaultValue: NightModeTabHelper?
  }

  var nightMode: NightModeTabHelper? {
    get { self[NightModeTabHelperKey.self] }
    set { self[NightModeTabHelperKey.self] = newValue }
  }
}

class NightModeTabHelper: TabObserver {
  private weak var tab: (any TabState)?
  private var prefObserver: PrefObserver?

  init(tab: some TabState) {
    self.tab = tab

    let prefObserver = PrefObserver { [weak self] in
      self?.isEnabled = Preferences.General.nightModeEnabled.value
    }
    Preferences.General.nightModeEnabled.observe(from: prefObserver)
    self.prefObserver = prefObserver

    tab.addObserver(self)
  }

  deinit {
    tab?.removeObserver(self)
  }

  private var isEnabled: Bool = Preferences.General.nightModeEnabled.value {
    didSet {
      guard let url = tab?.visibleURL else { return }
      if isEnabled, !Self.isNightModeBlockedURL(url) {
        enable()
      } else {
        disable()
      }
    }
  }

  private func enable() {
    guard let tab else { return }
    DarkReaderScriptHandler.enable(for: tab)
  }

  private func disable() {
    guard let tab else { return }
    DarkReaderScriptHandler.disable(for: tab)
  }

  static func isNightModeBlockedURL(_ url: URL) -> Bool {
    // The reason we use `normalizedHost` is because we want to keep the eTLD+1
    // IE: (search.brave.com instead of brave.com)
    guard let urlHost = url.normalizedHost(), let registry = url.publicSuffix else {
      return false
    }

    // Remove the `registry` so we get `search.brave` instead of `search.brave.com`
    // We get amazon instead of amazon.co.uk
    // mail.proton instead of mail.proton.com
    let domainName = urlHost.dropLast(registry.count).trimmingCharacters(
      in: CharacterSet(charactersIn: ".")
    )

    // Site domains that should NOT inject night mode
    let siteList = Set([
      // Popular sites that already have dark mode
      "twitter", "youtube", "twitch", "soundcloud", "github", "netflix", "imdb", "mail.proton", "x",
      "search.brave", "google", "qwant", "startpage", "duckduckgo",

      // Sites that have webcompat issues with DarkReader, document the particular webcompat issue
      // with each site so that it may be tested again when DarkReader is updated.
      "developer.apple",  // Some images disappear
      "cineplex",  // Trailer thumbnails disappear
    ])

    return siteList.contains(domainName)
  }

  private func refreshNightMode() {
    isEnabled = Preferences.General.nightModeEnabled.value
  }

  // MARK: - TabObserver

  func tabDidCommitNavigation(_ tab: some TabState) {
    refreshNightMode()
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }

  // MARK: -

  private class PrefObserver: NSObject, PreferencesObserver {
    var notify: () -> Void
    init(notify: @escaping () -> Void) {
      self.notify = notify
    }
    func preferencesDidChange(for key: String) {
      notify()
    }
  }
}
