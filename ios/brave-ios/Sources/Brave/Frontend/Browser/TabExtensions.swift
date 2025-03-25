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

extension Tab {
  var displayTitle: String {
    if let displayTabTitle = fetchDisplayTitle(using: url, title: title) {
      return displayTabTitle
    }

    // When picking a display title. Tabs with sessionData are pending a restore so show their old title.
    // To prevent flickering of the display title. If a tab is restoring make sure to use its lastTitle.
    if let url = self.url, InternalURL(url)?.isAboutHomeURL ?? false, !isRestoring {
      return Strings.Hotkey.newTabTitle
    }

    if let url = self.url, !InternalURL.isValid(url: url),
      let shownUrl = url.displayURL?.absoluteString, isWebViewCreated
    {
      return shownUrl
    }

    guard let lastTitle = lastTitle, !lastTitle.isEmpty else {
      // FF uses url?.displayURL?.absoluteString ??  ""
      if let title = url?.absoluteString {
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
    if let url = url, InternalURL(url)?.isAboutHomeURL == true {
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
      if let url = url, url.isWebPage() {
        return url
      }
    } else {
      if let tabUrl = url, tabUrl.isWebPage() {
        return tabUrl
      } else if let fetchedTab = SessionTab.from(tabId: id), fetchedTab.url?.isWebPage() == true {
        return url
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
    webContentView?.isUserInteractionEnabled = false
    if animated {
      UIView.animate(
        withDuration: 0.25,
        animations: { () -> Void in
          self.webContentView?.alpha = 0.0
        }
      )
    } else {
      webContentView?.alpha = 0.0
    }
  }

  func showContent(_ animated: Bool = false) {
    webContentView?.isUserInteractionEnabled = true
    if animated {
      UIView.animate(
        withDuration: 0.25,
        animations: { () -> Void in
          self.webContentView?.alpha = 1.0
        }
      )
    } else {
      webContentView?.alpha = 1.0
    }
  }

  func stopMediaPlayback() {
    data.browserData?.miscDelegate?.stopMediaPlayback(self)
  }
}

// MARK: - Brave Search

extension Tab {
  /// Call the api on the Brave Search website and passes the fallback results to it.
  /// Important: This method is also called when there is no fallback results
  /// or when the fallback call should not happen at all.
  /// The website expects the iOS device to always call this method(blocks on it).
  func injectResults() {
    DispatchQueue.main.async {
      // If the backup search results happen before the Brave Search loads
      // The method we pass data to is undefined.
      // For such case we do not call that method or remove the search backup manager.

      self.evaluateSafeJavaScript(
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

        if let url = self.url,
          BraveSearchManager.isValidURL(url),
          let result = self.braveSearchManager?.fallbackQueryResult
        {
          queryResult = result
        }

        self.evaluateSafeJavaScript(
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
}

// MARK: - Brave SKU
extension Tab {
  func injectLocalStorageItem(key: String, value: String) {
    self.evaluateSafeJavaScript(
      functionName: "localStorage.setItem",
      args: [key, value],
      contentWorld: BraveSkusScriptHandler.scriptSandbox
    )
  }
}
