// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Data
import FaviconModels
import Foundation
import OSLog
import Shared
import UIKit
import Web

extension TabState {
  var displayTitle: String {
    if let url = self.visibleURL, InternalURL(url)?.isAboutHomeURL ?? false {
      return Strings.Hotkey.newTabTitle
    }

    if let displayTabTitle = fetchDisplayTitle(using: visibleURL, title: title) {
      return displayTabTitle
    }

    if let url = self.visibleURL, !InternalURL.isValid(url: url),
      let shownUrl = url.displayURL?.absoluteString, isWebViewCreated
    {
      return shownUrl
    }

    guard let lastTitle = data.browserData?.lastTitle, !lastTitle.isEmpty else {
      // FF uses url?.displayURL?.absoluteString ??  ""
      if let title = visibleURL?.absoluteString {
        return title
      } else if let tab = SessionTab.from(tabId: id) {
        if tab.title.isEmpty {
          return Strings.Hotkey.newTabTitle
        }
        return tab.title
      }

      return ""
    }

    return lastTitle
  }

  var displayFavicon: Favicon? {
    if let url = visibleURL, InternalURL(url)?.isAboutHomeURL == true {
      return Favicon(
        image: UIImage(sharedNamed: "brave.logo"),
        isMonogramImage: false,
        backgroundColor: .clear
      )
    }
    return favicon
  }

  /// This property is for fetching the actual URL for the Tab
  /// In private browsing the URL is in memory but this is not the case for normal mode
  /// For Normal  Mode Tab information is fetched using Tab ID from
  var fetchedURL: URL? {
    if isPrivate {
      if let url = visibleURL, url.isWebPage() {
        return url
      }
    } else {
      if let tabUrl = visibleURL, tabUrl.isWebPage() {
        return tabUrl
      } else if let fetchedTab = SessionTab.from(tabId: id), fetchedTab.url?.isWebPage() == true {
        return visibleURL
      }
    }

    return nil
  }

  func fetchDisplayTitle(using url: URL?, title: String?) -> String? {
    if let tabTitle = title, !tabTitle.isEmpty {
      var displayTitle = tabTitle

      // Checking host is "localhost" || host == "127.0.0.1"
      // or hostless URL (iOS forwards hostless URLs (e.g., http://:6571) to localhost.)
      // DisplayURL will retrieve original URL even it is redirected to Error Page
      if let isLocal = url?.displayURL?.isLocal, isLocal {
        displayTitle = ""
      }

      return displayTitle
    }

    return nil
  }

  func hideContent(_ animated: Bool = false) {
    view.isUserInteractionEnabled = false
    if animated {
      UIView.animate(
        withDuration: 0.25,
        animations: { () -> Void in
          self.view.alpha = 0.0
        }
      )
    } else {
      view.alpha = 0.0
    }
  }

  func showContent(_ animated: Bool = false) {
    view.isUserInteractionEnabled = true
    if animated {
      UIView.animate(
        withDuration: 0.25,
        animations: { () -> Void in
          self.view.alpha = 1.0
        }
      )
    } else {
      view.alpha = 1.0
    }
  }

  func stopMediaPlayback() {
    data.browserData?.miscDelegate?.stopMediaPlayback(self)
  }

  var containsWebPage: Bool {
    if let url = visibleURL {
      let isHomeURL = InternalURL(url)?.isAboutHomeURL
      return url.isWebPage() && isHomeURL != true
    }

    return false
  }
}

// MARK: - Brave Search

extension TabState {
  /// Call the api on the Brave Search website and passes the fallback results to it.
  /// Important: This method is also called when there is no fallback results
  /// or when the fallback call should not happen at all.
  /// The website expects the iOS device to always call this method(blocks on it).
  func injectResults() {
    DispatchQueue.main.async {
      // If the backup search results happen before the Brave Search loads
      // The method we pass data to is undefined.
      // For such case we do not call that method or remove the search backup manager.

      self.evaluateJavaScript(
        functionName: "window.onFetchedBackupResults === undefined",
        contentWorld: BraveSearchScriptHandler.scriptSandbox,
        asFunction: false
      ) { (result, error) in

        if let error = error {
          Logger.module.error(
            "onFetchedBackupResults existence check error: \(error.localizedDescription, privacy: .public)"
          )
        }

        guard let methodUndefined = result as? Bool else {
          Logger.module.error(
            "onFetchedBackupResults existence check, failed to unwrap bool result value"
          )
          return
        }

        if methodUndefined {
          Logger.module.info("Search Backup results are ready but the page has not been loaded yet")
          return
        }

        var queryResult = "null"

        if let url = self.visibleURL,
          BraveSearchManager.isValidURL(url),
          let result = self.braveSearchManager?.fallbackQueryResult
        {
          queryResult = result
        }

        self.evaluateJavaScript(
          functionName: "window.onFetchedBackupResults",
          args: [queryResult],
          contentWorld: BraveSearchScriptHandler.scriptSandbox,
          escapeArgs: false
        )

        // Cleanup
        self.braveSearchManager = nil
      }
    }
  }

  func replaceLocation(with url: URL) {
    let apostropheEncoded = "%27"
    let safeUrl = url.absoluteString.replacingOccurrences(of: "'", with: apostropheEncoded)
    evaluateJavaScript(
      functionName: "location.replace",
      args: ["'\(safeUrl)'"],
      contentWorld: .defaultClient,
      escapeArgs: false
    )
  }
}

// MARK: - Brave SKU
extension TabState {
  func injectLocalStorageItem(key: String, value: String) {
    self.evaluateJavaScript(
      functionName: "localStorage.setItem",
      args: [key, value],
      contentWorld: BraveSkusScriptHandler.scriptSandbox
    )
  }
}

extension SecureContentState {
  public var shouldDisplayWarning: Bool {
    switch self {
    case .unknown, .invalidCertificate, .missingSSL, .mixedContent:
      return true
    case .localhost, .secure:
      return false
    }
  }
}
