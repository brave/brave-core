// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import Data
import SnapKit
import UIKit
import Web

/// Hosts the favorites and search-results screens shown while editing the URL bar, showing the
/// favorites screen while the query is empty and the search screen once the user starts typing.
///
/// Consolidates what were previously two independently-managed child controllers
/// (`FavoritesViewController` and `SearchViewController`, plus a `SearchLoader`) into a single
/// container so the browser only needs to build and track one object while editing the URL.
class SearchContainerViewController: UIViewController {
  private let favoritesController: FavoritesViewController
  private let searchController: SearchViewController
  private let searchLoader: SearchLoader

  var isUsingBottomBar: Bool = false {
    didSet {
      searchController.isUsingBottomBar = isUsingBottomBar
    }
  }

  init(
    tabManager: TabManager,
    bookmarkManager: BookmarkManager,
    historyAPI: BraveHistoryAPI,
    searchEngines: SearchEngines,
    privateBrowsingManager: PrivateBrowsingManager,
    isAIChatAvailable: Bool,
    isPlaylistAvailable: Bool,
    searchDelegate: SearchViewControllerDelegate,
    autocompleteSuggestionHandler: @escaping (String?) -> Void,
    bookmarkAction: @escaping (Favorite, BookmarksAction) -> Void,
    recentSearchAction: @escaping (RecentSearch?, Bool) -> Void
  ) {
    let searchController = SearchViewController(
      with: .init(
        isPrivate: privateBrowsingManager.isPrivateBrowsing,
        isAIChatAvailable: isAIChatAvailable,
        isPlaylistAvailable: isPlaylistAvailable,
        searchEngines: searchEngines
      ),
      browserColors: privateBrowsingManager.browserColors
    )
    searchController.searchDelegate = searchDelegate
    self.searchController = searchController

    let searchLoader = SearchLoader(
      historyAPI: historyAPI,
      bookmarkManager: bookmarkManager,
      tabManager: tabManager
    )
    searchLoader.addListener(searchController)
    searchLoader.autocompleteSuggestionHandler = autocompleteSuggestionHandler
    self.searchLoader = searchLoader

    self.favoritesController = FavoritesViewController(
      privateBrowsingManager: privateBrowsingManager,
      defaultSearchEngine: searchEngines.defaultEngine(
        forType: privateBrowsingManager.isPrivateBrowsing ? .privateMode : .standard
      ),
      bookmarkAction: bookmarkAction,
      recentSearchAction: recentSearchAction
    )

    super.init(nibName: nil, bundle: nil)
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    view.backgroundColor = .clear

    addChild(favoritesController)
    view.addSubview(favoritesController.view)
    favoritesController.view.snp.makeConstraints { $0.edges.equalTo(view) }
    favoritesController.didMove(toParent: self)

    addChild(searchController)
    view.addSubview(searchController.view)
    searchController.view.snp.makeConstraints { $0.edges.equalTo(view) }
    searchController.didMove(toParent: self)

    searchController.isUsingBottomBar = isUsingBottomBar
    searchController.setupSearchEngineList()

    showSearchResults(false)
  }

  /// Applies the given insets to both the favorites and search-results children so their content
  /// clears the toolbar/header.
  func applyAdditionalSafeAreaInsets(_ insets: UIEdgeInsets) {
    favoritesController.additionalSafeAreaInsets = insets
    searchController.additionalSafeAreaInsets = insets
  }

  /// Toggles between the favorites screen and the search-results screen.
  func showSearchResults(_ show: Bool) {
    searchController.view.isHidden = !show
    favoritesController.view.isHidden = show
  }

  /// Drives the search results/suggestions for the given field text.
  func setSearchQuery(query: String, showSearchSuggestions: Bool) {
    searchController.setSearchQuery(
      query: query,
      showSearchSuggestions: showSearchSuggestions
    )
  }

  /// The text used to drive the history/bookmarks/open-tabs autocomplete suggestions.
  var searchLoaderQuery: String {
    get { searchLoader.query }
    set { searchLoader.query = newValue }
  }

  /// Forwards a hardware-keyboard command (arrow keys, etc.) to the search results.
  func handleSearchKeyCommands(sender: UIKeyCommand) {
    searchController.handleKeyCommands(sender: sender)
  }

  func presentQuickSearchEnginesViewController(profile: LegacyBrowserProfile) {
    let quickSearchEnginesViewController = SearchQuickEnginesViewController(profile: profile)
    quickSearchEnginesViewController.navigationItem.leftBarButtonItem =
      UIBarButtonItem(
        title: Strings.close,
        style: .done,
        target: self,
        action: #selector(dismissQuickSearchEngines)
      )
    quickSearchEnginesViewController.delegate = searchController

    let navVC = ModalSettingsNavigationController(
      rootViewController: quickSearchEnginesViewController
    )
    self.present(navVC, animated: true, completion: nil)
  }

  @objc private func dismissQuickSearchEngines() {
    dismiss(animated: true) {
      self.reloadSearchEngineLayout()
    }
  }

  /// Re-lays out the quick-search-engine row (e.g. after the settings screen is dismissed).
  func reloadSearchEngineLayout() {
    searchController.layoutSearchEngineScrollView()
  }
}
