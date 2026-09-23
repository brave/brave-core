// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import Data
import SnapKit
import SpeechRecognition
import UIKit
import Web

/// Events forwarded from the URL input field to the browser.
protocol SearchContainerViewControllerDelegate: AnyObject {
  func searchContainer(_ container: SearchContainerViewController, didSubmitText text: String)
  func searchContainerDidCancel(_ container: SearchContainerViewController)
  func searchContainerDidTapPasteAndGo(_ container: SearchContainerViewController)
  func searchContainerDidTapQRCode(_ container: SearchContainerViewController)
  func searchContainerDidTapVoiceSearch(_ container: SearchContainerViewController)
}

/// The fullscreen editing surface presented when the user taps the URL bar.
///
/// Owns the editable URL/search input field and builds and hosts both the favorites and
/// search-results screens as children, showing the favorites screen while the query is empty and the
/// search screen once the user starts typing. Both children fill the entire browser so nothing behind
/// them is visible, including when a hardware keyboard is connected.
class SearchContainerViewController: UIViewController {
  weak var delegate: SearchContainerViewControllerDelegate?

  let inputBar: SearchURLBarInputView
  private let favoritesController: FavoritesViewController
  private let searchController: SearchViewController
  private let searchLoader: SearchLoader

  var isUsingBottomBar: Bool = false {
    didSet {
      searchController.isUsingBottomBar = isUsingBottomBar
      updateInputBarLayout()
      updateChildInsets()
    }
  }

  init(
    tabManager: TabManager,
    bookmarkManager: BookmarkManager,
    historyAPI: BraveHistoryAPI,
    searchEngines: SearchEngines,
    privateBrowsingManager: PrivateBrowsingManager,
    speechRecognizer: SpeechRecognizer,
    isAIChatAvailable: Bool,
    isPlaylistAvailable: Bool,
    searchDelegate: SearchViewControllerDelegate,
    delegate: SearchContainerViewControllerDelegate,
    bookmarkAction: @escaping (Favorite, BookmarksAction) -> Void,
    recentSearchAction: @escaping (RecentSearch?, Bool) -> Void
  ) {
    self.delegate = delegate

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
    self.searchLoader = searchLoader

    self.favoritesController = FavoritesViewController(
      privateBrowsingManager: privateBrowsingManager,
      defaultSearchEngine: searchEngines.defaultEngine(
        forType: privateBrowsingManager.isPrivateBrowsing ? .privateMode : .standard
      ),
      bookmarkAction: bookmarkAction,
      recentSearchAction: recentSearchAction
    )

    self.inputBar = SearchURLBarInputView(
      privateBrowsingManager: privateBrowsingManager,
      speechRecognizer: speechRecognizer
    )

    super.init(nibName: nil, bundle: nil)

    searchLoader.autocompleteSuggestionHandler = { [weak self] completion in
      self?.setAutocompleteSuggestion(completion)
    }
    inputBar.actionDelegate = self
    inputBar.textField.autocompleteDelegate = self
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    view.backgroundColor = .clear

    // Track the view's bottom edge (not the safe area) when the keyboard is down so the bottom-bar
    // input field's background extends to the bottom of the screen behind the home indicator.
    view.keyboardLayoutGuide.usesBottomSafeArea = false

    addChild(searchController)
    view.addSubview(searchController.view)
    searchController.view.snp.makeConstraints { $0.edges.equalTo(view) }
    searchController.didMove(toParent: self)

    addChild(favoritesController)
    view.addSubview(favoritesController.view)
    favoritesController.view.snp.makeConstraints { $0.edges.equalTo(view) }
    favoritesController.didMove(toParent: self)

    view.addSubview(inputBar)

    // Favorites shows first; the search screen appears once the user types.
    searchController.view.isHidden = true
    favoritesController.view.isHidden = false

    searchController.isUsingBottomBar = isUsingBottomBar
    // Called after the search view is in the hierarchy so its engine row can be laid out.
    searchController.setupSearchEngineList()
    updateInputBarLayout()
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    updateChildInsets()
  }

  private func updateInputBarLayout() {
    inputBar.snp.remakeConstraints { make in
      make.leading.trailing.equalTo(view)
      if isUsingBottomBar {
        make.bottom.equalTo(view.keyboardLayoutGuide.snp.top)
      } else {
        make.top.equalTo(view)
      }
    }
  }

  /// Insets the child screens so their content clears the input bar (which floats above the keyboard
  /// in bottom-bar mode or sits at the top otherwise). The children handle keyboard avoidance for
  /// their own content.
  private func updateChildInsets() {
    let inputBarHeight =
      inputBar.bounds.height - inputBar.safeAreaInsets.top - inputBar.safeAreaInsets.bottom
    let insets: UIEdgeInsets =
      isUsingBottomBar
      ? .init(top: 0, left: 0, bottom: inputBarHeight, right: 0)
      : .init(top: inputBarHeight, left: 0, bottom: 0, right: 0)
    favoritesController.additionalSafeAreaInsets = insets
    searchController.additionalSafeAreaInsets = insets
  }

  private func showSearchResults(_ show: Bool) {
    searchController.view.isHidden = !show
    favoritesController.view.isHidden = show
  }

  // MARK: - Editing

  /// Focuses the input field, seeding it with the given text. `search: true` treats the text as a
  /// search query; `false` treats it as a URL being edited. `selectAll` selects the seeded text.
  func beginEditing(with text: String?, search: Bool, selectAll: Bool) {
    // Defer becoming first responder so the field is in the hierarchy first and the search query
    // fires while the field is editing.
    DispatchQueue.main.async { [weak self] in
      guard let self else { return }
      self.inputBar.becomeFirstResponder()
      self.inputBar.setLocation(text, search: search)
    }
    if selectAll {
      DispatchQueue.main.async { [weak self] in
        self?.inputBar.selectAllText()
      }
    }
  }

  /// Seeds the input field from an external source (recent search, suggestion long-press).
  func applyExternalQuery(_ text: String, search: Bool) {
    inputBar.setLocation(text, search: search)
  }

  func setAutocompleteSuggestion(_ suggestion: String?) {
    inputBar.setAutocompleteSuggestion(suggestion)
  }

  /// Forwards a hardware-keyboard command (arrow keys, etc.) to the search results.
  func handleSearchKeyCommands(sender: UIKeyCommand) {
    searchController.handleKeyCommands(sender: sender)
  }

  /// The delegate for the quick-search-engines settings screen.
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

  /// Drives the search results/suggestions for the current field text.
  private func updateSearchResults(for text: String) {
    if text.isEmpty {
      searchController.setSearchQuery(query: "", showSearchSuggestions: false)
      searchLoader.query = ""
    } else {
      Task {
        let showSuggestions = await URLBarHelper.shared.shouldShowSearchSuggestions(
          using: inputBar.textField.lastReplacement ?? "",
          isPasting: inputBar.textField.isPasting
        )
        searchController.setSearchQuery(query: text, showSearchSuggestions: showSuggestions)
        searchLoader.query = text.lowercased()
      }
    }
  }
}

// MARK: - AutocompleteTextFieldDelegate

extension SearchContainerViewController: AutocompleteTextFieldDelegate {
  func autocompleteTextFieldShouldReturn(_ autocompleteTextField: AutocompleteTextField) -> Bool {
    guard let text = autocompleteTextField.text else { return true }
    if !text.trimmingCharacters(in: .whitespaces).isEmpty {
      delegate?.searchContainer(self, didSubmitText: text)
      return true
    }
    return false
  }

  func autocompleteTextField(
    _ autocompleteTextField: AutocompleteTextField,
    didEnterText text: String
  ) {
    showSearchResults(!text.isEmpty)
    inputBar.updateLocationBarRightView(showToolbarActions: text.isEmpty)
    updateSearchResults(for: text)
  }

  func autocompleteTextField(
    _ autocompleteTextField: AutocompleteTextField,
    didDeleteAutoSelectedText text: String
  ) {
    inputBar.updateLocationBarRightView(showToolbarActions: text.isEmpty)
  }

  func autocompleteTextFieldDidBeginEditing(_ autocompleteTextField: AutocompleteTextField) {
    autocompleteTextField.highlightAll()
    inputBar.updateLocationBarRightView(
      showToolbarActions: autocompleteTextField.text?.isEmpty == true
    )
  }

  func autocompleteTextFieldShouldClear(_ autocompleteTextField: AutocompleteTextField) -> Bool {
    showSearchResults(false)
    inputBar.updateLocationBarRightView(showToolbarActions: true)
    updateSearchResults(for: "")
    return true
  }

  func autocompleteTextFieldDidCancel(_ autocompleteTextField: AutocompleteTextField) {
    delegate?.searchContainerDidCancel(self)
  }
}

// MARK: - SearchURLBarInputViewDelegate

extension SearchContainerViewController: SearchURLBarInputViewDelegate {
  func searchURLBarInputViewDidTapCancel(_ inputView: SearchURLBarInputView) {
    delegate?.searchContainerDidCancel(self)
  }

  func searchURLBarInputViewDidTapPasteAndGo(_ inputView: SearchURLBarInputView) {
    delegate?.searchContainerDidTapPasteAndGo(self)
  }

  func searchURLBarInputViewDidTapQRCode(_ inputView: SearchURLBarInputView) {
    delegate?.searchContainerDidTapQRCode(self)
  }

  func searchURLBarInputViewDidTapVoiceSearch(_ inputView: SearchURLBarInputView) {
    delegate?.searchContainerDidTapVoiceSearch(self)
  }
}
