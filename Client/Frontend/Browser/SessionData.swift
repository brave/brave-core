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
    
    struct Keys {
        static let currentPage = "currentPage"
        static let history = "history"
        static let lastUsedTime = "lastUsedTime"
        static let urls = "url"
    }

    var jsonDictionary: [String: Any] {
        return [
            SessionData.Keys.currentPage: String(self.currentPage),
            SessionData.Keys.lastUsedTime: String(self.lastUsedTime),
            SessionData.Keys.urls: urls.map { $0.absoluteString }
        ]
    }

    /**
        Creates a new SessionData object representing a serialized tab.

        - parameter currentPage:     The active page index. Must be in the range of (-N, 0],
                                where 1-N is the first page in history, and 0 is the last.
        - parameter urls:            The sequence of URLs in this tab's session history.
        - parameter lastUsedTime:    The last time this tab was modified.
    **/
    init(currentPage: Int, urls: [URL], lastUsedTime: Timestamp) {
        self.currentPage = currentPage
        self.urls = SessionData.updateSessionURLs(urls: urls)
        self.lastUsedTime = lastUsedTime

        assert(!urls.isEmpty, "Session has at least one entry")
        assert(currentPage > -urls.count && currentPage <= 0, "Session index is valid")
    }

    required init?(coder: NSCoder) {
        self.currentPage = coder.decodeInteger(forKey: SessionData.Keys.currentPage)
        self.urls = coder.decodeObject(of: [NSURL.self], forKey: "urls") as? [URL] ?? []
        self.lastUsedTime = UInt64(coder.decodeInt64(forKey: SessionData.Keys.lastUsedTime))
    }

    func encode(with coder: NSCoder) {
        coder.encode(currentPage, forKey: SessionData.Keys.currentPage)
        coder.encode(urls, forKey: SessionData.Keys.urls)
        coder.encode(Int64(lastUsedTime), forKey: SessionData.Keys.lastUsedTime)
    }
    
    // This is not a fully direct mapping, but rather an attempt to reconcile data differences, primarily used for tab restoration
    var savedTabData: SavedTab {
        let urlStrings = jsonDictionary[SessionData.Keys.urls] as? [String] ?? []
        let currentURL = urlStrings[(currentPage < 0 ? max(urlStrings.count-1, 0) : currentPage)]
        
        return SavedTab(id: "InvalidId", title: nil, url: currentURL, isSelected: false, order: -1, screenshot: nil, history: urlStrings, historyIndex: Int16(currentPage))
    }
    
    static var supportsSecureCoding: Bool {
        return true
    }
    
    private static func updateSessionURLs(urls: [URL]) -> [URL] {
        return urls.compactMap { url in
            if PrivilegedRequest.isWebServerRequest(url: url),
               PrivilegedRequest.isPrivileged(url: url),
               let strippedURL = PrivilegedRequest.removePrivileges(url: url) {
                return strippedURL
            }
            return url
        }
    }
}
