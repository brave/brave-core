// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import Storage
import Web
import WebKit
import os.log

extension TabDataValues {
  private struct PageMetadataTabHelperKey: TabDataKey {
    static var defaultValue: PageMetadataTabHelper?
  }

  var pageMetadataHelper: PageMetadataTabHelper? {
    get { self[PageMetadataTabHelperKey.self] }
    set { self[PageMetadataTabHelperKey.self] = newValue }
  }
}

/// Value types representing a page's metadata
struct PageMetadata: Decodable {
  let siteURL: String
  let mediaURL: String?
  let title: String?
  let description: String?
  let type: String?
  let providerName: String?
  let faviconURL: String?
  let largeIconURL: String?
  let keywords: Set<String>?
  let search: Link?
  let feeds: [Link]

  enum CodingKeys: String, CodingKey {
    case mediaURL = "image"
    case siteURL = "url"
    case title
    case description
    case type
    case providerName = "provider"
    case faviconURL = "icon"
    case largeIconURL = "largeIcon"
    case keywords
    case search
    case feeds
  }

  init(
    siteURL: String,
    mediaURL: String?,
    title: String?,
    description: String?,
    type: String?,
    providerName: String?,
    faviconURL: String? = nil,
    largeIconURL: String? = nil,
    keywords: Set<String>? = nil,
    search: Link? = nil,
    feeds: [Link] = []
  ) {
    self.siteURL = siteURL
    self.mediaURL = mediaURL
    self.title = title
    self.description = description
    self.type = type
    self.providerName = providerName
    self.faviconURL = faviconURL
    self.largeIconURL = largeIconURL
    self.keywords = keywords
    self.search = search
    self.feeds = feeds
  }

  struct Link: Decodable {
    var href: String
    var title: String
  }
}

class PageMetadataTabHelper: TabObserver {
  private weak var tab: (any TabState)?
  private(set) var metadata: PageMetadata?

  init(tab: some TabState) {
    self.tab = tab
    tab.addObserver(self)
  }

  deinit {
    tab?.removeObserver(self)
  }

  private func fetchMetadata(in tab: some TabState) {
    // Get the metadata out of the page-metadata-parser, and into a type safe struct as soon
    // as possible.
    guard let url = tab.visibleURL, url.isWebPage(includeDataURIs: false),
      !InternalURL.isValid(url: url)
    else {
      metadata = nil
      return
    }

    tab.evaluateJavaScript(
      functionName: "__firefox__.metadata && __firefox__.metadata.getMetadata()",
      contentWorld: .defaultClient,
      asFunction: false
    ) { [self] (result, error) in
      guard error == nil else {
        // TabEvent.post(.pageMetadataNotAvailable, for: tab)
        metadata = nil
        return
      }

      guard let dict = result as? [String: Any],
        let data = try? JSONSerialization.data(withJSONObject: dict, options: [])
      else {
        Logger.module.debug("Page contains no metadata!")
        metadata = nil
        return
      }

      do {
        let pageMetadata = try JSONDecoder().decode(PageMetadata.self, from: data)
        metadata = pageMetadata
      } catch {
        Logger.module.error(
          "Failed to parse metadata: \(error.localizedDescription, privacy: .public)"
        )
        // To avoid issues where `pageMetadata` points to the last website to successfully
        // parse metadata, set to nil
        metadata = nil
      }
    }
  }

  func tabDidStartNavigation(_ tab: some TabState) {
    metadata = nil
  }

  func tabDidFinishNavigation(_ tab: some TabState) {
    fetchMetadata(in: tab)
  }

  func tabDidUpdateURL(_ tab: some TabState) {
    fetchMetadata(in: tab)
  }

  func tabWillBeDestroyed(_ tab: some TabState) {
    tab.removeObserver(self)
  }
}
