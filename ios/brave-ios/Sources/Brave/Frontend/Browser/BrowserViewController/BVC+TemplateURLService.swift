// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Web

extension BrowserViewController {
  /// The `TemplateURLService` used for search when `kUseChromiumSearchEngines` is enabled
  private var searchTemplateURLService: (any TemplateURLService)? {
    guard let templateURLService = TemplateURLServiceFactory.get(profile: profileController.profile)
    else {
      return nil
    }
    // Loading is a no-op if the service has already loaded. Prior to loading, the service still
    // provides the default search providers from prefs.
    templateURLService.load()
    return templateURLService
  }

  /// The default search engine to use for the currently selected tab
  private func defaultSearchProvider(
    in templateURLService: any TemplateURLService
  ) -> TemplateURL? {
    let isPrivate =
      tabManager.selectedTab?.isPrivate ?? privateBrowsingManager.isPrivateBrowsing
    return isPrivate
      ? templateURLService.defaultPrivateSearchProvider
      : templateURLService.defaultSearchProvider
  }

  /// Returns the search results page URL for the given query using the default search engine of
  /// the selected tab.
  func templateURLSearchURL(
    for query: String,
    isBraveSearchPromotion: Bool,
    isWidgetSearchAttribution: Bool
  ) -> URL? {
    guard let templateURLService = searchTemplateURLService else {
      return nil
    }
    var templateURL = defaultSearchProvider(in: templateURLService)
    if isBraveSearchPromotion,
      let braveSearch = templateURLService.templateURL(forPrepopulateID: .brave)
    {
      templateURL = braveSearch
    }
    guard
      var searchURL = templateURL?.urlRef.replaceSearchTerms(
        .init(searchTerms: query),
        searchTermsData: templateURLService.searchTermsData
      )
    else {
      return nil
    }
    if isBraveSearchPromotion {
      searchURL = searchURL.withQueryParam("action", value: "makeDefault")
    }
    if isWidgetSearchAttribution {
      searchURL = OpenSearchEngine.applyWidgetSearchAttribution(to: searchURL) ?? searchURL
    }
    return searchURL
  }

  /// Returns the search terms from `url` if it is a search results page of the default search
  /// engine of the selected tab.
  func templateURLSearchTerms(from url: URL) -> String? {
    guard let templateURLService = searchTemplateURLService,
      let templateURL = defaultSearchProvider(in: templateURLService),
      let searchTerms = templateURL.extractSearchTerms(
        from: url,
        searchTermsData: templateURLService.searchTermsData
      ),
      !searchTerms.isEmpty
    else {
      return nil
    }
    return searchTerms
  }
}
