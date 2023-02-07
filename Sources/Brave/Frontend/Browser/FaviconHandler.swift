/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import Storage
import SDWebImage
import UIKit
import Favicon

class FaviconHandler {
  static let maximumFaviconSize = 1 * 1024 * 1024  // 1 MiB file size limit

  private var tabObservers: TabObservers!
  private let backgroundQueue = OperationQueue()

  init() {
    self.tabObservers = registerFor(.didLoadPageMetadata, queue: backgroundQueue)
  }

  deinit {
    unregister(tabObservers)
  }

  @MainActor func loadFaviconURL(
    _ faviconURL: String,
    forTab tab: Tab
  ) async throws -> Favicon {
    guard let currentURL = tab.url else {
      throw FaviconError.noImagesFound
    }
    
    let favicon = try await FaviconFetcher.loadIcon(url: currentURL, kind: .smallIcon, persistent: !tab.isPrivate)
    tab.favicons.append(favicon)
    return favicon
  }
}

extension FaviconHandler: TabEventHandler {
  func tab(_ tab: Tab, didLoadPageMetadata metadata: PageMetadata) {
    tab.favicons.removeAll(keepingCapacity: false)
    Task { @MainActor in
      if let iconURL = metadata.largeIconURL {
        let favicon = try await loadFaviconURL(iconURL, forTab: tab)
        TabEvent.post(.didLoadFavicon(favicon), for: tab)
      } else if let iconURL = metadata.faviconURL {
        let favicon = try await loadFaviconURL(iconURL, forTab: tab)
        TabEvent.post(.didLoadFavicon(favicon), for: tab)
      }
      // No favicon fetched from metadata, trying base domain's standard favicon location.
      else if let baseURL = tab.url?.domainURL {
        let favicon = try await loadFaviconURL(
          baseURL.appendingPathComponent("favicon.ico").absoluteString,
          forTab: tab)
        TabEvent.post(.didLoadFavicon(favicon), for: tab)
      }
    }
  }
}
