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
import os.log

// MARK: - SearchSuggestionDataSourceDelegate

protocol SearchSuggestionDataSourceDelegate: AnyObject {
  func searchSuggestionDataSourceReloaded()
}

class SearchSuggestionDataSource {

  // MARK: SearchListSection

  enum SearchListSection: Int, CaseIterable {
    case quickBar
    case searchSuggestionsOptIn
    case searchSuggestions
    case findInPage
    case openTabsAndHistoryAndBookmarks
    case aiChat
  }

  let tabType: TabType
  let maxSearchSuggestions = 5
  var suggestions = [String]()
  private let maxPeriodBraveSearchPromotion = 15
  private var suggestClient: SearchSuggestClient?

  var delegate: SearchSuggestionDataSourceDelegate?

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
      searchEngines?.defaultEngine(forType: tabType == .private ? .privateMode : .standard)?
      .engineID
      == quickSearchEngines.first?.engineID

    if quickSearchEngines.count == 1 {
      return !isDefaultEngineQuickEngine
    }

    return quickSearchEngines.count > 1
  }

  var availableSections: [SearchListSection] {
    var sections = [SearchListSection]()
    sections.append(.quickBar)

    if !tabType.isPrivate && searchEngines?.shouldShowSearchSuggestionsOptIn == true {
      sections.append(.searchSuggestionsOptIn)
    }

    if !tabType.isPrivate && searchEngines?.shouldShowSearchSuggestions == true {
      sections.append(.searchSuggestions)
    }
    sections.append(.findInPage)

    if searchEngines?.shouldShowBrowserSuggestions == true {
      sections.append(.openTabsAndHistoryAndBookmarks)
    }

    if !tabType.isPrivate && Preferences.AIChat.autocompleteSuggestionsEnabled.value
      && FeatureList.kAIChat.enabled
    {
      sections.append(.aiChat)
    }

    return sections
  }

  var braveSearchPromotionAvailable: Bool {
    guard Preferences.Review.launchCount.value > 1,
      searchEngines?.defaultEngine(forType: tabType == .private ? .privateMode : .standard)?
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

  init(forTabType tabType: TabType, searchEngines: SearchEngines?) {
    self.tabType = tabType
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
    if !tabType.isPrivate,
      let userAgent = SearchViewController.userAgent,
      let engines = searchEngines?.defaultEngine(
        forType: tabType == .private ? .privateMode : .standard
      )
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
