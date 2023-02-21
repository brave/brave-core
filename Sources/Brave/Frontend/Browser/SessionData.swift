/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation

import Shared
import Data

class SessionData: NSObject, NSSecureCoding {
  let currentPage: Int
  let urls: [URL]
  let lastUsedTime: Timestamp
  let isPrivate: Bool

  struct Keys {
    static let currentPage = "currentPage"
    static let history = "history"
    static let lastUsedTime = "lastUsedTime"
    static let urls = "url"
    static let isPrivate = "isPrivate"
  }

  var jsonDictionary: [String: Any] {
    return [
      SessionData.Keys.currentPage: String(self.currentPage),
      SessionData.Keys.lastUsedTime: String(self.lastUsedTime),
      SessionData.Keys.urls: urls.map { $0.absoluteString },
      SessionData.Keys.isPrivate: self.isPrivate,
    ]
  }

  /**
        Creates a new SessionData object representing a serialized tab.

        - parameter currentPage:     The active page index. Must be in the range of (-N, 0],
                                where 1-N is the first page in history, and 0 is the last.
        - parameter urls:            The sequence of URLs in this tab's session history.
        - parameter lastUsedTime:    The last time this tab was modified.
    **/
  init(currentPage: Int, urls: [URL], lastUsedTime: Timestamp, isPrivate: Bool) {
    self.currentPage = currentPage
    self.urls = SessionData.updateSessionURLs(urls: urls)
    self.lastUsedTime = lastUsedTime
    self.isPrivate = isPrivate

    assert(!urls.isEmpty, "Session has at least one entry")
    assert(currentPage > -urls.count && currentPage <= 0, "Session index is valid")
  }

  required init?(coder: NSCoder) {
    self.currentPage = coder.decodeInteger(forKey: SessionData.Keys.currentPage)
    self.urls = coder.decodeObject(of: [NSURL.self], forKey: "urls") as? [URL] ?? []
    self.lastUsedTime = UInt64(coder.decodeInt64(forKey: SessionData.Keys.lastUsedTime))
    self.isPrivate = coder.decodeBool(forKey: SessionData.Keys.isPrivate)
  }

  func encode(with coder: NSCoder) {
    coder.encode(currentPage, forKey: SessionData.Keys.currentPage)
    coder.encode(urls, forKey: SessionData.Keys.urls)
    coder.encode(Int64(lastUsedTime), forKey: SessionData.Keys.lastUsedTime)
    coder.encode(isPrivate, forKey: SessionData.Keys.isPrivate)
  }

  // This is not a fully direct mapping, but rather an attempt to reconcile data differences, primarily used for tab restoration
  var savedTabData: SavedTab {
    let urlStrings = jsonDictionary[SessionData.Keys.urls] as? [String] ?? []
    let isPrivate = jsonDictionary[SessionData.Keys.isPrivate] as? Bool ?? false
    let currentURL = urlStrings[(currentPage < 0 ? max(urlStrings.count - 1, 0) : currentPage)]

    return SavedTab(id: "InvalidId", title: nil, url: currentURL, isSelected: false, order: -1, screenshot: nil, history: urlStrings, historyIndex: Int16(currentPage), isPrivate: isPrivate)
  }

  static var supportsSecureCoding: Bool {
    return true
  }

  /// PR: https://github.com/mozilla-mobile/firefox-ios/pull/4387
  /// Commit: https://github.com/mozilla-mobile/firefox-ios/commit/8b1450fbeb87f1f559a2f8e42971c715dc96bcaf
  /// InternalURL helps  encapsulate all internal scheme logic for urls rather than using URL extension. Extensions to built-in classes should be more minimal that what was being done previously.
  /// This migration was required mainly for above PR which is related to a PI request that reduces security risk. Also, this particular method helps in cleaning up / migrating old localhost:6571 URLs to internal: SessionData urls
  static func updateSessionURLs(urls: [URL]) -> [URL] {
    return urls.compactMap { url in
      var url = url
      let port = AppConstants.webServerPort
      [
        ("http://localhost:\(port)/errors/error.html?url=", "\(InternalURL.baseUrl)/\(SessionRestoreHandler.path)?url="),
        // ("http://localhost:\(port)/reader-mode?url=", "\(InternalURL.baseUrl)/\(ReaderModeHandler.path)?url=")
      ].forEach {
        oldItem, newItem in
        if url.absoluteString.hasPrefix(oldItem) {
          var urlStr = url.absoluteString.replacingOccurrences(of: oldItem, with: newItem)
          let comp = urlStr.components(separatedBy: newItem)
          if comp.count > 2 {
            // get the last instance of incorrectly nested urls
            urlStr = newItem + (comp.last ?? "")
            assertionFailure("SessionData urls have nested internal links, investigate: [\(url.absoluteString)]")
          }
          url = URL(string: urlStr) ?? url
        }
      }

      if let internalUrl = InternalURL(url), internalUrl.isAuthorized, let stripped = URL(string: internalUrl.stripAuthorization) {
        return stripped
      }

      return url
    }
  }
}
