// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import Shared
import WebKit

public class DarkReaderScriptHandler: TabContentScript {
  #if USE_NIGHTMODE_COLOURS
  private static let configuration = [
    "brightness": 100,
    "contrast": 90,
    "sephia": 10,
  ]
  #else
  private static let configuration: [String: Int] = [:]
  #endif

  static let scriptName = "DarkReaderScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .world(name: "DarkReaderContentWorld")
  static let userScript: WKUserScript? = {
    guard var script = loadUserScript(named: scriptName) else {
      return nil
    }
    return WKUserScript(
      source: secureScript(
        handlerName: messageHandlerName,
        securityToken: scriptId,
        script: script
      ),
      injectionTime: .atDocumentStart,
      forMainFrameOnly: true,
      in: scriptSandbox
    )
  }()

  func tab(
    _ tab: Tab,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    // Do nothing. There's no message handling for Dark-Reader
  }

  /// Enables DarkReader
  static func enable(for tab: Tab) {
    // This is needed to ensure that CORS fetches work correctly, otherwise you get this error:
    // "Embedded Dark Reader cannot access a cross-origin resource"
    tab.evaluateSafeJavaScript(
      functionName: "DarkReader.setFetchMethod(window.fetch)",
      contentWorld: Self.scriptSandbox,
      asFunction: false
    )
    tab.evaluateSafeJavaScript(
      functionName: "DarkReader.enable",
      args: configuration.isEmpty ? [] : [configuration],
      contentWorld: Self.scriptSandbox
    )
  }

  /// Disables DarkReader
  static func disable(for tab: Tab) {
    tab.evaluateSafeJavaScript(
      functionName: "DarkReader.disable",
      args: [],
      contentWorld: Self.scriptSandbox
    )
  }

  /// - Parameter enabled - If true, automatically listens for system's dark-mode vs. light-mode. If false, disables listening.
  static func auto(enabled: Bool = true, for tab: Tab) {
    if enabled {
      // This is needed to ensure that CORS fetches work correctly, otherwise you get this error:
      // "Embedded Dark Reader cannot access a cross-origin resource"
      tab.evaluateSafeJavaScript(
        functionName: "DarkReader.setFetchMethod(window.fetch)",
        contentWorld: Self.scriptSandbox,
        asFunction: false
      )
      tab.evaluateSafeJavaScript(
        functionName: "DarkReader.auto",
        args: configuration.isEmpty ? [] : [configuration],
        contentWorld: Self.scriptSandbox
      )
    } else {
      tab.evaluateSafeJavaScript(
        functionName: "DarkReader.auto",
        args: [false],
        contentWorld: Self.scriptSandbox
      )
    }
  }

  static func set(tabManager: TabManager, enabled: Bool) {
    Preferences.General.nightModeEnabled.value = enabled

    for tab in tabManager.allTabs {
      if let fetchedTabURL = tab.fetchedURL, !isNightModeBlockedURL(fetchedTabURL) {
        tab.nightMode = enabled
      }
    }
  }
}

extension DarkReaderScriptHandler {
  // Check if the website is a night mode blocked site
  public static func isNightModeBlockedURL(_ url: URL) -> Bool {
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
      "twitter", "youtube", "twitch",
      "soundcloud", "github", "netflix",
      "imdb", "mail.proton", "amazon",
      "x", "search.brave",

      // Search Engines
      "search.brave", "google", "qwant",
      "startpage", "duckduckgo", "presearch",

      // Dev sites
      "macrumors", "9to5mac", "developer.apple",

      // Casual sites
      "wowhead", "xbox", "thegamer",
      "cineplex", "starwars",
    ])

    return siteList.contains(domainName)
  }
}
