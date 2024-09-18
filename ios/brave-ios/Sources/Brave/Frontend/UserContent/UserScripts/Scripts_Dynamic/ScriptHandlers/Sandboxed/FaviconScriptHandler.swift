// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Favicon
import Foundation
import Shared
import WebKit
import os.log

class FaviconScriptHandler: NSObject, TabContentScript {
  static let scriptName = "FaviconScript"
  static let scriptId = UUID().uuidString
  static let messageHandlerName = "\(scriptName)_\(messageUUID)"
  static let scriptSandbox: WKContentWorld = .defaultClient
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
      injectionTime: .atDocumentEnd,
      forMainFrameOnly: true,
      in: scriptSandbox
    )
  }()

  func tab(
    _ tab: Tab,
    receivedScriptMessage message: WKScriptMessage,
    replyHandler: @escaping (Any?, String?) -> Void
  ) {
    defer { replyHandler(nil, nil) }

    Task { @MainActor in
      // Assign default favicon
      tab.favicon = Favicon.default

      guard let webView = message.webView,
        let url = webView.url
      else {
        return
      }

      // The WebView has a valid URL
      // Attempt to fetch the favicon from cache
      let isPrivate = tab.isPrivate
      tab.favicon = await FaviconFetcher.getIconFromCache(for: url) ?? Favicon.default

      // If this is an internal page, we don't fetch favicons for such pages from Brave-Core
      guard !InternalURL.isValid(url: url),
        !(InternalURL(url)?.isSessionRestore ?? false)
      else {
        return
      }

      // Update the favicon for this tab, from Brave-Core
      tab.faviconDriver?.webView(webView, scriptMessage: message) { [weak tab] iconUrl, icon in
        FaviconScriptHandler.updateFavicon(
          tab: tab,
          url: url,
          isPrivate: isPrivate,
          icon: icon,
          iconUrl: iconUrl
        )
      }
    }
  }

  private static func updateFavicon(
    tab: Tab?,
    url: URL,
    isPrivate: Bool,
    icon: UIImage?,
    iconUrl: URL?
  ) {
    if let icon = icon {
      if let iconUrl = iconUrl {
        Logger.module.debug(
          "Fetched Favicon: \(iconUrl.absoluteString), for page: \(url.absoluteString)"
        )
      } else {
        Logger.module.debug("Fetched Favicon for page: \(url.absoluteString)")
      }

      // If the icon is too small, we don't want to cache it.
      // It's better to show monogram or bundled icons.
      if icon.size.width < CGFloat(FaviconLoader.Sizes.desiredMedium.rawValue)
        || icon.size.height < CGFloat(FaviconLoader.Sizes.desiredMedium.rawValue)
      {
        return
      }

      Task { @MainActor in
        // Always fetch the favicon from the database instead of the `icon` parameter
        // This will allow us to always get high-res icons, and only fallback to the `icon` parameter
        // If the icon couldn't be fetched at all.
        var favicon = try? await FaviconRenderer.loadIcon(for: url, persistent: !isPrivate)
        if favicon == nil {
          favicon = await Favicon.renderImage(icon, backgroundColor: .clear, shouldScale: true)
        }

        guard let tab = tab, let favicon = favicon else { return }

        // We can only cache favicons for non-private tabs
        await FaviconFetcher.updateCache(favicon, for: url, persistent: !isPrivate)

        tab.favicon = favicon
        TabEvent.post(.didLoadFavicon(favicon), for: tab)
      }
    } else {
      if let iconUrl = iconUrl {
        Logger.module.error(
          "Failed fetching Favicon: \(iconUrl.absoluteString), for page: \(url.absoluteString)"
        )
      } else {
        Logger.module.error("Website: \(url.absoluteString), has no Favicon")
      }

      Task { @MainActor in
        let favicon = try await FaviconFetcher.monogramIcon(url: url, persistent: !isPrivate)
        await FaviconFetcher.updateCache(favicon, for: url, persistent: true)

        guard let tab = tab else { return }
        tab.favicon = favicon
        TabEvent.post(.didLoadFavicon(nil), for: tab)
      }
    }
  }
}
