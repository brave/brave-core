// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Favicon
import Foundation
import SDWebImage
import Shared
import Storage
import UIKit
import Web

extension TabDataValues {
  private struct FaviconTabHelperKey: TabDataKey {
    static var defaultValue: FaviconTabHelper?
  }

  var faviconTabHelper: FaviconTabHelper? {
    get { self[FaviconTabHelperKey.self] }
    set { self[FaviconTabHelperKey.self] = newValue }
  }
}

class FaviconTabHelper: TabObserver {
  static let maximumFaviconSize = 1 * 1024 * 1024  // 1 MiB file size limit

  private weak var tab: (any TabState)?

  init(tab: some TabState) {
    self.tab = tab
    tab.addObserver(self)
  }

  deinit {
    tab?.removeObserver(self)
  }

  @MainActor func loadFaviconURL(
    _ url: URL,
    forTab tab: some TabState
  ) async throws -> Favicon {
    let favicon = try await FaviconFetcher.loadIcon(
      url: url,
      kind: .smallIcon,
      persistent: !tab.isPrivate
    )
    tab.favicon = favicon
    return favicon
  }

  func tabDidUpdateURL(_ tab: some TabState) {
    if let currentURL = tab.visibleURL {
      Task { @MainActor in
        if let favicon = await FaviconFetcher.getIconFromCache(for: currentURL) {
          tab.favicon = favicon
        } else {
          tab.favicon = Favicon.default
        }
        let favicon = try await loadFaviconURL(currentURL, forTab: tab)
      }
    } else {
      tab.favicon = Favicon.default
    }
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }
}
