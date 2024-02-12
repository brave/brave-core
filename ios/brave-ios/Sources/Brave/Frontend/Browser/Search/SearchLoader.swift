/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Shared
import Storage
import Data
import BraveCore
import Preferences

/// Shared data source for the SearchViewController and the URLBar domain completion.
/// Since both of these use the same query, we can perform the query once and dispatch the results.
class SearchLoader: Loader<[Site], SearchViewController> {
  private let frequencyQuery: FrequencyQuery

  var autocompleteSuggestionHandler: ((String) -> Void)?

  init(historyAPI: BraveHistoryAPI, bookmarkManager: BookmarkManager, tabManager: TabManager) {
    frequencyQuery = FrequencyQuery(historyAPI: historyAPI, bookmarkManager: bookmarkManager, tabManager: tabManager)
  }

  var query: String = "" {
    didSet {
      // Browser suggestions preference control frequencey query over browser items
      // like Open Tabs, Bookmarks, History. Disabling this preference should prevent
      // data fetch from the sources
      guard Preferences.Search.showBrowserSuggestions.value else {
        load([])
        return
      }
      
      if query.isEmpty {
        load([])
        return
      }

      frequencyQuery.sitesByFrequency(containing: query) { [weak self] result in
        guard let self = self else { return }

        self.load(Array(result))

        // If the new search string is not longer than the previous
        // we don't need to find an autocomplete suggestion.
        if oldValue.count > self.query.count {
          return
        }

        for site in result {
          if let completion = self.completionForURL(site.url) {
            self.autocompleteSuggestionHandler?(completion)
            return
          }
        }
      }
    }
  }

  fileprivate func completionForURL(_ url: String) -> String? {
    // Private Browsing Mode (PBM) should *not* show items from normal mode History etc
    // when search suggestions is not enabled
    if Preferences.Privacy.privateBrowsingOnly.value, !Preferences.Search.showSuggestions.value {
      return nil
    }

    // Extract the pre-path substring from the URL. This should be more efficient than parsing via
    // NSURL since we need to only look at the beginning of the string.
    // Note that we won't match non-HTTP(S) URLs.
    guard let urlBeforePathRegex = try? NSRegularExpression(pattern: "^https?://([^/]+)/", options: []),
      let match = urlBeforePathRegex.firstMatch(in: url, options: [], range: NSRange(location: 0, length: url.count))
    else {
      return nil
    }

    // If the pre-path component (including the scheme) starts with the query, just use it as is.
    var prePathURL = (url as NSString).substring(with: match.range(at: 0))
    if prePathURL.hasPrefix(query) {
      // Trailing slashes in the autocompleteTextField cause issues with Swype keyboard. Bug 1194714
      if prePathURL.hasSuffix("/") {
        prePathURL.remove(at: prePathURL.index(before: prePathURL.endIndex))
      }
      return prePathURL
    }

    // Otherwise, find and use any matching domain.
    // To simplify the search, prepend a ".", and search the string for ".query".
    // For example, for http://en.m.wikipedia.org, domainWithDotPrefix will be ".en.m.wikipedia.org".
    // This allows us to use the "." as a separator, so we can match "en", "m", "wikipedia", and "org",
    let domain = (url as NSString).substring(with: match.range(at: 1))
    return completionForDomain(domain)
  }

  fileprivate func completionForDomain(_ domain: String) -> String? {
    let domainWithDotPrefix: String = ".\(domain)"
    if let range = domainWithDotPrefix.range(of: ".\(query)", options: .caseInsensitive, range: nil, locale: nil) {
      // We don't actually want to match the top-level domain ("com", "org", etc.) by itself, so
      // so make sure the result includes at least one ".".
      let matchedDomain = String(domainWithDotPrefix.suffix(from: domainWithDotPrefix.index(range.lowerBound, offsetBy: 1)))
      if matchedDomain.contains(".") {
        return matchedDomain
      }
    }

    return nil
  }
}
