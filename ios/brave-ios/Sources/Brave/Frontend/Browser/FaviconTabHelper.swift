// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Favicon
import Foundation
import OSLog
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
  private weak var tab: (any TabState)?

  private var favicon: Favicon = .default

  var displayFavicon: Favicon? {
    guard let tab else { return nil }
    if let url = tab.visibleURL, url.isNewTabURL {
      return Favicon(
        image: UIImage(sharedNamed: "brave.logo"),
        isMonogramImage: false,
        backgroundColor: .clear
      )
    }
    if FeatureList.kUseProfileWebViewConfiguration.enabled {
      if let faviconStatusImage = tab.faviconStatus?.image {
        return Favicon(image: faviconStatusImage, isMonogramImage: false, backgroundColor: .clear)
      }
    }
    return favicon
  }

  init(tab: some TabState) {
    self.tab = tab
    tab.addObserver(self)

    Task { @MainActor in
      if let url = tab.visibleURL {
        if let icon = await FaviconFetcher.getIconFromCache(for: url) {
          self.favicon = icon
        } else {
          self.favicon = try await FaviconFetcher.loadIcon(
            url: url,
            persistent: !tab.isPrivate
          )
        }
      }
    }
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
      persistent: !tab.isPrivate
    )
    return favicon
  }

  private func updateFavicon(tab: some TabState) {
    if let currentURL = tab.visibleURL {
      Task { @MainActor in
        if let favicon = await FaviconFetcher.getIconFromCache(for: currentURL) {
          self.favicon = favicon
        } else {
          favicon = Favicon.default
        }
        favicon = try await loadFaviconURL(currentURL, forTab: tab)
      }
    } else {
      favicon = Favicon.default
    }
  }

  @MainActor
  private func updateFaviconAndUpdateCache(tab: some TabState) async {
    guard let url = tab.visibleURL else { return }
    if let faviconStatus = tab.faviconStatus, let icon = faviconStatus.image {
      // If the icon is too small, we don't want to cache it.
      // It's better to show monogram or bundled icons.
      if icon.size.width < CGFloat(FaviconLoader.Sizes.desiredMedium.rawValue)
        || icon.size.height < CGFloat(FaviconLoader.Sizes.desiredMedium.rawValue)
      {
        return
      }
      // Always fetch the favicon from the database instead of the `icon` parameter
      // This will allow us to always get high-res icons, and only fallback to the `icon` parameter
      // If the icon couldn't be fetched at all.
      var favicon = try? await FaviconRenderer.loadIcon(for: url, persistent: !tab.isPrivate)
      if favicon == nil {
        favicon = await Favicon.renderImage(icon, backgroundColor: .clear, shouldScale: true)
      }

      if let favicon {
        // We can only cache favicons for non-private tabs
        await FaviconFetcher.updateCache(favicon, for: url, persistent: !tab.isPrivate)

        tab.faviconTabHelper?.favicon = favicon
      }
    } else {
      do {
        let favicon = try await FaviconFetcher.monogramIcon(url: url, persistent: !tab.isPrivate)
        await FaviconFetcher.updateCache(favicon, for: url, persistent: true)
        tab.faviconTabHelper?.favicon = favicon
      } catch {
        Logger.module.error("Failed to create monogram icon for \(url)")
      }
    }
  }

  func setFavicon(_ favicon: Favicon) {
    self.favicon = favicon
  }

  // MARK: - TabObserver

  func tabDidUpdateURL(_ tab: some TabState) {
    updateFavicon(tab: tab)
  }

  func tabDidUpdateFaviconStatus(_ tab: some TabState) {
    if FeatureList.kUseProfileWebViewConfiguration.enabled {
      Task { @MainActor in
        await updateFaviconAndUpdateCache(tab: tab)
      }
    }
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }
}
