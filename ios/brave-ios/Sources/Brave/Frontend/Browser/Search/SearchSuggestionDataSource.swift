// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import AIChat
import BraveCore
import BraveUI
import Foundation
import Preferences
import Shared
import Web
import os.log

// MARK: - SearchSuggestionDataSourceDelegate

protocol SearchSuggestionDataSourceDelegate: AnyObject {
  func searchSuggestionDataSourceReloaded()
}

class SearchSuggestionDataSource {

  // MARK: SearchListSection

  enum SearchListSection: Int, CaseIterable {
    case searchSuggestionsOptIn
    case searchSuggestions
    case braveSearchPromotion
    case findInPage
    case openTabsAndHistoryAndBookmarks
  }

  let isPrivate: Bool
  let maxSearchSuggestions = 5
  var suggestions = [String]()
  private let maxPeriodBraveSearchPromotion = 15
  private var suggestClient: SearchSuggestClient?

  weak var delegate: SearchSuggestionDataSourceDelegate?

  var searchQuery: String = "" {
    didSet {
      // Reload the tableView to show the updated text in each engine.
      delegate?.searchSuggestionDataSourceReloaded()
    }
  }

  let searchEngines: SearchEngines?

  var quickSearchEngines: [OpenSearchEngine] {
    guard let searchEngines = searchEngines?.quickSearchEngines else { return [] }
    return searchEngines
  }

  // If the user only has a single quick search engine, it is also their default one.
  // In that case, we count it as if there are no quick suggestions to show
  // Unless Default Search Engine is different than Quick Search Engine
  var hasQuickSearchEngines: Bool {
    let isDefaultEngineQuickEngine =
      searchEngines?.defaultEngine(forType: isPrivate ? .privateMode : .standard)?
      .engineID
      == quickSearchEngines.first?.engineID

    if quickSearchEngines.count == 1 {
      return !isDefaultEngineQuickEngine
    }

    return quickSearchEngines.count > 1
  }

  var isAIChatAvailable: Bool {
    !tabType.isPrivate
      && Preferences.AIChat.autocompleteSuggestionsEnabled.value
      && FeatureList.kAIChat.enabled
  }

  var braveSearchPromotionAvailable: Bool {
    guard Preferences.Review.launchCount.value > 1,
      searchEngines?.defaultEngine(forType: isPrivate ? .privateMode : .standard)?
        .shortName != OpenSearchEngine.EngineNames.brave,
      let braveSearchPromotionLaunchDate = Preferences.BraveSearch.braveSearchPromotionLaunchDate
        .value,
      Preferences.BraveSearch.braveSearchPromotionCompletionState.value
        != BraveSearchPromotionState.dismissed.rawValue,
      Preferences.BraveSearch.braveSearchPromotionCompletionState.value
        != BraveSearchPromotionState.maybeLaterSameSession.rawValue
    else {
      return false
    }

    let rightNow = Date()
    let nextShowDate = braveSearchPromotionLaunchDate.addingTimeInterval(
      AppConstants.isOfficialBuild
        ? maxPeriodBraveSearchPromotion.days : maxPeriodBraveSearchPromotion.minutes
    )

    if rightNow > nextShowDate {
      return false
    }

    return true
  }

  // MARK: - Initialization

  init(isPrivate: Bool, searchEngines: SearchEngines?) {
    self.isPrivate = isPrivate
    self.searchEngines = searchEngines
  }

  func querySuggestClient() {
    // Do not query suggestions if user is not opt_ed in
    if !Preferences.Search.showSuggestions.value {
      Logger.module.info("Suggestions are not enabled")
      return
    }

    cancelPendingSuggestionsRequests()

    let localSearchQuery = searchQuery.lowercased()
    if localSearchQuery.isEmpty || searchEngines?.shouldShowSearchSuggestionsOptIn == true
      || localSearchQuery.looksLikeAURL()
    {
      suggestions = []
      delegate?.searchSuggestionDataSourceReloaded()

      return
    }

    suggestClient?.query(
      localSearchQuery,
      callback: { [weak self] suggestions, error in
        guard let self = self else { return }

        self.delegate?.searchSuggestionDataSourceReloaded()
        if let error = error {
          let isSuggestClientError = error.domain == SearchSuggestClient.errorDomain

          switch error.code {
          case NSURLErrorCancelled where error.domain == NSURLErrorDomain:
            // Request was cancelled. Do nothing.
            break
          case SearchSuggestClient.invalidEngineErrorCode where isSuggestClientError:
            // Engine does not support search suggestions. Do nothing.
            break
          case SearchSuggestClient.invalidResponseErrorCode where isSuggestClientError:
            Logger.module.error("Error: Invalid search suggestion data")
          default:
            Logger.module.error("Error: \(error.description)")
          }
        } else if let suggestionList = suggestions {
          self.suggestions = suggestionList
        }

        // If there are no suggestions, just use whatever the user typed.
        if suggestions?.isEmpty ?? true {
          self.suggestions = [localSearchQuery]
        }

        // Reload the tableView to show the new list of search suggestions.
        self.delegate?.searchSuggestionDataSourceReloaded()
      }
    )
  }

  func cancelPendingSuggestionsRequests() {
    suggestClient?.cancelPendingRequest()
  }

  func setupSearchClient() {
    // Show the default search engine first.
    if !isPrivate,
      let userAgent = SearchViewController.userAgent,
      let engines = searchEngines?.defaultEngine(forType: .standard)
    {
      suggestClient = SearchSuggestClient(searchEngine: engines, userAgent: userAgent)
    }
  }

}

// MARK: - String Extension

extension String {
  func looksLikeAURL() -> Bool {
    // The assumption here is that if the user is typing in a forward slash and there are no spaces
    // involved, it's going to be a URL. If we type a space, any url would be invalid.
    // See https://bugzilla.mozilla.org/show_bug.cgi?id=1192155 for additional details.
    return self.contains("/") && !self.contains(" ")
  }
}
