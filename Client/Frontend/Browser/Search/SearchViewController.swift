/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared
import Storage
import BraveShared
import Data

// MARK: - SearchViewControllerDelegate

protocol SearchViewControllerDelegate: AnyObject {
    func searchViewController(_ searchViewController: SearchViewController, didSubmit query: String)
    func searchViewController(_ searchViewController: SearchViewController, didSelectURL url: URL)
    func searchViewController(_ searchViewController: SearchViewController, didLongPressSuggestion suggestion: String)
    func presentSearchSettingsController()
    func searchViewController(_ searchViewController: SearchViewController, didHighlightText text: String, search: Bool)
    func searchViewController(_ searchViewController: SearchViewController, shouldFindInPage query: String)
    func searchViewControllerAllowFindInPage() -> Bool
}

// MARK: - SearchViewController

class SearchViewController: SiteTableViewController, LoaderListener {

    // MARK: SearchViewControllerUX
    
    private struct SearchViewControllerUX {
        static let searchEngineScrollViewBorderColor = UIColor.black.withAlphaComponent(0.2).cgColor

        // TODO: This should use ToolbarHeight in BVC. Fix this when we create a shared theming file.
        static let engineButtonHeight: Float = 44
        static let engineButtonWidth = engineButtonHeight * 1.4
        static let engineButtonBackgroundColor = UIColor.clear.cgColor

        static let searchEngineTopBorderWidth = 0.5
        static let searchImageHeight: Float = 44
        static let searchImageWidth: Float = 24
        static let searchButtonMargin: CGFloat = 8

        static let faviconSize: CGFloat = 29
        static let iconBorderColor = UIColor(white: 0, alpha: 0.1)
        static let iconBorderWidth: CGFloat = 0.5
        static let maxSearchSuggestions = 6
    }

    // MARK: SearchListSection
    private enum SearchListSection: Int, CaseIterable {
        case quickBar
        case searchSuggestionsOptIn
        case searchSuggestions
        case findInPage
        case bookmarksAndHistory
    }

    // MARK: Properties
    
    private let tabType: TabType
    private var suggestClient: SearchSuggestClient?
    private var keyboardHeightForOrientation: (portrait: CGFloat, landscape: CGFloat) = (0, 0)

    // Views for displaying the bottom scrollable search engine list. searchEngineScrollView is the
    // scrollable container; searchEngineScrollViewContent contains the actual set of search engine buttons.
    private let searchEngineScrollView = ButtonScrollView().then { scrollView in
        scrollView.decelerationRate = .fast
        scrollView.showsVerticalScrollIndicator = false
        scrollView.showsHorizontalScrollIndicator = false
        scrollView.clipsToBounds = false
        scrollView.backgroundColor = .braveBackground
        let border = UIView.separatorLine
        scrollView.addSubview(border)
        border.snp.makeConstraints {
            $0.bottom.equalTo(scrollView.snp.top)
            $0.leading.trailing.equalTo(scrollView.frameLayoutGuide)
        }
    }
    private let searchEngineScrollViewContent = UIView().then {
        $0.backgroundColor = .braveBackground
    }
    
    private lazy var bookmarkedBadge: UIImage = {
        return #imageLiteral(resourceName: "bookmarked_passive")
    }()

    private var suggestions = [String]()
    private lazy var suggestionLongPressGesture = UILongPressGestureRecognizer(target: self, action: #selector(onSuggestionLongPressed(_:)))
    
    static var userAgent: String?
    var searchDelegate: SearchViewControllerDelegate?

    var searchEngines: SearchEngines? {
        didSet {
            suggestClient?.cancelPendingRequest()

            // Query and reload the table with new search suggestions.
            querySuggestClient()

            // Show the default search engine first.
            if !tabType.isPrivate {
                let userAgent = SearchViewController.userAgent ?? "FxSearch"
                if let engines = searchEngines?.defaultEngine() {
                    suggestClient = SearchSuggestClient(searchEngine: engines, userAgent: userAgent)
                }
            }

            // Reload the footer list of search engines.
            reloadSearchEngines()

            layoutSuggestionsOptInPrompt()
        }
    }

    private var quickSearchEngines: [OpenSearchEngine] {
        guard let engines = searchEngines else { return [] }
        var quickEngines = engines.quickSearchEngines

        // If we're not showing search suggestions, the default search engine won't be visible
        // at the top of the table. Show it with the others in the bottom search bar.
        if tabType.isPrivate || !engines.shouldShowSearchSuggestions {
            quickEngines?.insert(engines.defaultEngine(), at: 0)
        }

        guard let seachEngine = quickEngines else { return [] }
        return seachEngine
    }

    // If the user only has a single quick search engine, it is also their default one.
    // In that case, we count it as if there are no quick suggestions to show
    // Unless Default Search Engine is different than Quick Search Engine
    private var hasQuickSearchEngines: Bool {
        let isDefaultEngineQuickEngine = searchEngines?.defaultEngine().engineID == quickSearchEngines.first?.engineID

        if quickSearchEngines.count == 1 {
            return !isDefaultEngineQuickEngine
        }

        return quickSearchEngines.count > 1
    }

    var searchQuery: String = "" {
        didSet {
            // Reload the tableView to show the updated text in each engine.
            reloadData()
        }
    }
    
    private var availableSections: [SearchListSection] {
        var sections = [SearchListSection]()
        sections.append(.quickBar)
        
        if !tabType.isPrivate && searchEngines?.shouldShowSearchSuggestionsOptIn == true {
            sections.append(.searchSuggestionsOptIn)
        }
        
        if !tabType.isPrivate && searchEngines?.shouldShowSearchSuggestions == true {
            sections.append(.searchSuggestions)
        }
        sections.append(.findInPage)
        sections.append(.bookmarksAndHistory)
        return sections
    }

    // MARK: Lifecycle
    
    init(forTabType tabType: TabType) {
        self.tabType = tabType
        super.init(nibName: nil, bundle: nil)
    }

    required init?(coder aDecoder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    deinit {
      NotificationCenter.default.removeObserver(self, name: .dynamicFontChanged, object: nil)
    }
    
    override func viewDidLoad() {
        let blur = UIVisualEffectView(effect: UIBlurEffect(style: .light))
        view.addSubview(blur)

        super.viewDidLoad()
        setupSearchEngineScrollViewIfNeeded()

        KeyboardHelper.defaultHelper.addDelegate(self)

        blur.snp.makeConstraints { make in
            make.edges.equalTo(view)
        }
        
        tableView.keyboardDismissMode = .interactive
        tableView.separatorStyle = .none
        tableView.addGestureRecognizer(suggestionLongPressGesture)
        tableView.register(SearchSuggestionPromptCell.self, forCellReuseIdentifier: SearchSuggestionPromptCell.identifier)
        tableView.register(SuggestionCell.self, forCellReuseIdentifier: SuggestionCell.identifier)
        tableView.register(UITableViewCell.self, forCellReuseIdentifier: "default")
        NotificationCenter.default.addObserver(self, selector: #selector(dynamicFontChanged), name: .dynamicFontChanged, object: nil)
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        reloadSearchEngines()
        reloadData()
    }

    override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
        super.viewWillTransition(to: size, with: coordinator)
        // The height of the suggestions row may change, so call reloadData() to recalculate cell heights.
        coordinator.animate(alongsideTransition: { _ in
            self.tableView.reloadData()
        }, completion: nil)
    }

    private func animateSearchEnginesWithKeyboard(_ keyboardState: KeyboardState) {
        layoutSearchEngineScrollView()

        UIViewPropertyAnimator(duration: keyboardState.animationDuration, curve: keyboardState.animationCurve) {
            self.view.layoutIfNeeded()
        }
        .startAnimation()
    }

    @objc func dynamicFontChanged(_ notification: Notification) {
        if notification.name == .dynamicFontChanged {
            reloadData()
        }
    }
    
    // MARK: Internal

    private func setupSearchEngineScrollViewIfNeeded() {
        if !hasQuickSearchEngines { return }

        view.addSubview(searchEngineScrollView)
        searchEngineScrollView.addSubview(searchEngineScrollViewContent)

        layoutTable()
        layoutSearchEngineScrollView()

        searchEngineScrollViewContent.snp.makeConstraints { make in
            make.center.equalTo(searchEngineScrollView).priority(.low)
            // Left-align the engines on iphones, center on ipad
            if UIScreen.main.traitCollection.horizontalSizeClass == .compact {
                make.leading.equalTo(searchEngineScrollView).priority(.required)
            } else {
                make.leading.greaterThanOrEqualTo(searchEngineScrollView).priority(.required)
            }
            make.bottom.trailing.top.equalTo(searchEngineScrollView)
        }
    }

    private func layoutSearchEngineScrollView() {
        if !hasQuickSearchEngines { return }
        
        let keyboardHeight = KeyboardHelper.defaultHelper.currentState?.intersectionHeightForView(view) ?? 0

        if UIDevice.current.orientation.isLandscape, keyboardHeightForOrientation.landscape == 0 {
            keyboardHeightForOrientation.landscape = keyboardHeight
        } else if UIDevice.current.orientation.isPortrait, keyboardHeightForOrientation.portrait == 0 {
            keyboardHeightForOrientation.portrait = keyboardHeight
        }
            
        searchEngineScrollView.snp.remakeConstraints { make in
            make.leading.trailing.equalTo(view)
            
            if keyboardHeight == 0 {
                make.bottom.equalTo(view.safeArea.bottom)
            } else {
                let keyboardOrientationHeight = UIDevice.current.orientation.isPortrait
                    ? keyboardHeightForOrientation.portrait
                    : keyboardHeightForOrientation.landscape
                let keyboardOffset = UIDevice.isIpad ? keyboardHeight : keyboardOrientationHeight
                
                make.bottom.equalTo(view).offset(-(keyboardOffset))
            }
        }
    }
    
    private func layoutSuggestionsOptInPrompt() {
        if tabType.isPrivate || searchEngines?.shouldShowSearchSuggestionsOptIn == false {
            reloadData()
            return
        }
        
        layoutTable()
    }

    private func layoutTable() {
        tableView.snp.remakeConstraints { make in
            make.top.equalTo(view.snp.top)
            make.leading.trailing.equalTo(view)
            make.bottom.equalTo(hasQuickSearchEngines ? searchEngineScrollView.snp.top : self.view)
        }
    }
    
    override func reloadData() {
        querySuggestClient()
    }

    private func reloadSearchEngines() {
        searchEngineScrollViewContent.subviews.forEach { $0.removeFromSuperview() }
        var leftEdge = searchEngineScrollViewContent.snp.left

        // search settings icon
        let searchButton = UIButton()
        searchButton.setImage(#imageLiteral(resourceName: "quickSearch").template, for: [])
        searchButton.imageView?.contentMode = .center
        searchButton.layer.backgroundColor = SearchViewControllerUX.engineButtonBackgroundColor
        searchButton.addTarget(self, action: #selector(didClickSearchButton), for: .touchUpInside)
        searchButton.accessibilityLabel = Strings.searchSettingsButtonTitle
        searchButton.tintColor = .braveOrange

        searchButton.imageView?.snp.makeConstraints { make in
            make.width.height.equalTo(SearchViewControllerUX.searchImageWidth)
            return
        }

        searchEngineScrollViewContent.addSubview(searchButton)
        searchButton.snp.makeConstraints { make in
            make.size.equalTo(SearchViewControllerUX.faviconSize)
            // offset the left edge to align with search results
            make.left.equalTo(leftEdge).offset(SearchViewControllerUX.searchButtonMargin * 2)
            make.top.equalTo(searchEngineScrollViewContent).offset(SearchViewControllerUX.searchButtonMargin)
            make.bottom.equalTo(searchEngineScrollViewContent).offset(-SearchViewControllerUX.searchButtonMargin)
        }

        // search engines
        leftEdge = searchButton.snp.right
        for engine in quickSearchEngines {
            let engineButton = UIButton()
            engineButton.setImage(engine.image, for: [])
            engineButton.imageView?.contentMode = .scaleAspectFit
            engineButton.layer.backgroundColor = SearchViewControllerUX.engineButtonBackgroundColor
            engineButton.addTarget(self, action: #selector(didSelectEngine), for: .touchUpInside)
            engineButton.accessibilityLabel = String(format: Strings.searchEngineFormatText, engine.shortName)

            engineButton.imageView?.snp.makeConstraints { make in
                make.width.height.equalTo(SearchViewControllerUX.faviconSize)
                return
            }

            searchEngineScrollViewContent.addSubview(engineButton)
            engineButton.snp.makeConstraints { make in
                make.width.equalTo(SearchViewControllerUX.engineButtonWidth)
                make.left.equalTo(leftEdge)
                make.top.equalTo(searchEngineScrollViewContent)
                make.bottom.equalTo(searchEngineScrollViewContent)
                
                if engine === searchEngines?.quickSearchEngines.last {
                    make.right.equalTo(searchEngineScrollViewContent)
                }
            }
            leftEdge = engineButton.snp.right
        }
    }

    private func querySuggestClient() {
        suggestClient?.cancelPendingRequest()

        let localSearchQuery = searchQuery.lowercased()
        if localSearchQuery.isEmpty || searchEngines?.shouldShowSearchSuggestionsOptIn == true || localSearchQuery.looksLikeAURL() {
            suggestions = []
            tableView.reloadData()
            return
        }

        suggestClient?.query(localSearchQuery, callback: { [weak self] suggestions, error in
            guard let self = self else { return }

            self.tableView.reloadData()
            
            if let error = error {
                let isSuggestClientError = error.domain == SearchSuggestClientErrorDomain

                switch error.code {
                case NSURLErrorCancelled where error.domain == NSURLErrorDomain:
                    // Request was cancelled. Do nothing.
                    break
                case SearchSuggestClientErrorInvalidEngine where isSuggestClientError:
                    // Engine does not support search suggestions. Do nothing.
                    break
                case SearchSuggestClientErrorInvalidResponse where isSuggestClientError:
                    print("Error: Invalid search suggestion data")
                default:
                    print("Error: \(error.description)")
                }
            } else if let suggestionList = suggestions {
                self.suggestions = suggestionList
            }

            // If there are no suggestions, just use whatever the user typed.
            if suggestions?.isEmpty ?? true {
                self.suggestions = [localSearchQuery]
            }

            // Reload the tableView to show the new list of search suggestions.
            self.tableView.reloadData()
        })
    }

    func loader(dataLoaded data: [Site]) {
        self.data = Array(data.prefix(5))
        tableView.reloadData()
    }

    // MARK: Actions
    
    @objc func didSelectEngine(_ sender: UIButton) {
        // The UIButtons are the same cardinality and order as the array of quick search engines.
        // Subtract 1 from index to account for magnifying glass accessory.
        guard let index = searchEngineScrollViewContent.subviews.firstIndex(of: sender) else {
            assertionFailure()
            return
        }

        let engine = quickSearchEngines[index - 1]
        let localSearchQuery = searchQuery.lowercased()
        guard let url = engine.searchURLForQuery(localSearchQuery) else {
            assertionFailure()
            return
        }

        if !PrivateBrowsingManager.shared.isPrivateBrowsing {
            RecentSearch.addItem(type: .website, text: localSearchQuery, websiteUrl: url.absoluteString)
        }
        searchDelegate?.searchViewController(self, didSelectURL: url)
    }

    @objc func didClickSearchButton() {
        self.searchDelegate?.presentSearchSettingsController()
    }
    
    // MARK: UITableViewDelegate, UITableViewDataSource
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        guard let section = availableSections[safe: indexPath.section] else { return }
        
        switch section {
        case .quickBar:
            if !PrivateBrowsingManager.shared.isPrivateBrowsing {
                RecentSearch.addItem(type: .text, text: searchQuery, websiteUrl: nil)
            }
            searchDelegate?.searchViewController(self, didSubmit: searchQuery)
        case .searchSuggestionsOptIn: return
        case .searchSuggestions:
            // Assume that only the default search engine can provide search suggestions.
            let engine = searchEngines?.defaultEngine()
            let suggestion = suggestions[indexPath.row]

            var url = URIFixup.getURL(suggestion)
            if url == nil {
                url = engine?.searchURLForQuery(suggestion)
            }

            if let url = url {
                if !PrivateBrowsingManager.shared.isPrivateBrowsing {
                    RecentSearch.addItem(type: .website, text: suggestion, websiteUrl: url.absoluteString)
                }
                searchDelegate?.searchViewController(self, didSelectURL: url)
            }
        case .bookmarksAndHistory:
            let site = data[indexPath.row]
            if let url = URL(string: site.url) {
                searchDelegate?.searchViewController(self, didSelectURL: url)
            }
        case .findInPage:
            let localSearchQuery = searchQuery.lowercased()
            searchDelegate?.searchViewController(self, shouldFindInPage: localSearchQuery)
        }
    }

    override func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
        if let currentSection = availableSections[safe: indexPath.section] {
            switch currentSection {
            case .quickBar:
                return super.tableView(tableView, heightForRowAt: indexPath)
            case .searchSuggestionsOptIn:
                return 100.0
            case .searchSuggestions:
                return 44.0
            case .bookmarksAndHistory:
                return super.tableView(tableView, heightForRowAt: indexPath)
            case .findInPage:
                return super.tableView(tableView, heightForRowAt: indexPath)
            }
        }

        return 0
    }
    
    func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        guard let searchSection = availableSections[safe: section] else { return nil }
        
        switch searchSection {
        case .quickBar: return nil
        case .searchSuggestionsOptIn: return nil
        case .searchSuggestions:
            if let defaultSearchEngine = searchEngines?.defaultEngine() {
                if defaultSearchEngine.shortName.contains(Strings.searchSuggestionSectionTitleNoSearchFormat) ||
                    defaultSearchEngine.shortName.lowercased().contains("search") {
                    return defaultSearchEngine.shortName
                }
                return String(format: Strings.searchSuggestionSectionTitleFormat, defaultSearchEngine.shortName)
            }
            return Strings.searchSuggestionsSectionHeader
        case .bookmarksAndHistory: return Strings.searchHistorySectionHeader
        case .findInPage: return Strings.findOnPageSectionHeader
        }
    }
    
    func tableView(_ tableView: UITableView, willDisplayHeaderView view: UIView, forSection section: Int) {
        if let headerView = view as? UITableViewHeaderFooterView {
            headerView.textLabel?.textColor = .bravePrimary
        }
    }

    override func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
        guard let searchSection = availableSections[safe: section] else { return 0 }
        let headerHeight: CGFloat = 22
        
        switch searchSection {
        case .quickBar:
            return 0.0
        case .searchSuggestionsOptIn:
            return 0.0
        case .searchSuggestions:
            return suggestions.isEmpty ? 0 : headerHeight * 2.0
        case .bookmarksAndHistory: return data.isEmpty ? 0 : headerHeight
        case .findInPage:
            if let sd = searchDelegate, sd.searchViewControllerAllowFindInPage() {
                return headerHeight
            }
            return 0.0
        }
    }
    
    func tableView(_ tableView: UITableView, viewForFooterInSection section: Int) -> UIView? {
        return UIView()
    }
    
    func tableView(_ tableView: UITableView, heightForFooterInSection section: Int) -> CGFloat {
        guard let searchSection = availableSections[safe: section] else { return 0 }
        switch searchSection {
        case .quickBar:
            return 0.0
        case .searchSuggestionsOptIn:
            return 15.0
        case .searchSuggestions:
            return suggestions.isEmpty ? 0.0 : 15.0
        case .bookmarksAndHistory: return 15.0
        case .findInPage:
            return 0.0
        }
    }

    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        guard let section = availableSections[safe: indexPath.section] else {
            return UITableViewCell()
        }
        
        switch section {
        case .quickBar:
            let cell = TwoLineTableViewCell()
            cell.textLabel?.text = searchQuery
            cell.textLabel?.textColor = .bravePrimary
            cell.imageView?.image = #imageLiteral(resourceName: "search_bar_find_in_page_icon")
            cell.imageView?.contentMode = .center
            return cell
        case .searchSuggestionsOptIn:
            let cell = tableView.dequeueReusableCell(withIdentifier: SearchSuggestionPromptCell.identifier, for: indexPath)
            if let promptCell = cell as? SearchSuggestionPromptCell {
                promptCell.selectionStyle = .none
                promptCell.onOptionSelected = { [weak self] option in
                    guard let self = self else { return }

                    self.searchEngines?.shouldShowSearchSuggestions = option
                    self.searchEngines?.shouldShowSearchSuggestionsOptIn = false

                    if option {
                        self.querySuggestClient()
                    }
                    self.layoutSuggestionsOptInPrompt()
                    self.reloadSearchEngines()
                }
            }
            return cell
        case .searchSuggestions:
            let cell = tableView.dequeueReusableCell(withIdentifier: SuggestionCell.identifier, for: indexPath)
            if let suggestionCell = cell as? SuggestionCell {
                suggestionCell.setTitle(suggestions[indexPath.row])
                suggestionCell.separatorInset = UIEdgeInsets(top: 0.0, left: view.bounds.width, bottom: 0.0, right: -view.bounds.width)
                suggestionCell.openButtonActionHandler = { [weak self] in
                    guard let self = self else { return }
                    
                    let suggestion = self.suggestions[indexPath.row]
                    self.searchDelegate?.searchViewController(self, didLongPressSuggestion: suggestion)
                }
            }
            return cell

        case .bookmarksAndHistory:
            let cell = super.tableView(tableView, cellForRowAt: indexPath)
            let site = data[indexPath.row]
            if let cell = cell as? TwoLineTableViewCell {
                let isBookmark = site.bookmarked ?? false
                cell.textLabel?.textColor = .bravePrimary
                cell.setLines(site.title, detailText: site.url)
                cell.setRightBadge(isBookmark ? self.bookmarkedBadge : nil)
                cell.imageView?.contentMode = .scaleAspectFit
                cell.imageView?.layer.borderColor = SearchViewControllerUX.iconBorderColor.cgColor
                cell.imageView?.layer.borderWidth = SearchViewControllerUX.iconBorderWidth
                cell.imageView?.image = UIImage()
                cell.imageView?.loadFavicon(for: site.tileURL)
            }
            return cell
            
        case .findInPage:
            let cell = tableView.dequeueReusableCell(withIdentifier: "default", for: indexPath)
            cell.textLabel?.text = String(format: Strings.findInPageFormat, searchQuery)
            cell.textLabel?.textColor = .bravePrimary
            cell.textLabel?.numberOfLines = 2
            cell.textLabel?.font = .systemFont(ofSize: 15.0)
            cell.textLabel?.lineBreakMode = .byWordWrapping
            cell.textLabel?.textAlignment = .left
            cell.backgroundColor = .secondaryBraveBackground
            return cell
        }
    }

    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        guard let section = availableSections[safe: section] else {
            return 0
        }
        
        switch section {
        case .quickBar:
            return 1
        case .searchSuggestionsOptIn:
            return 1
        case .searchSuggestions:
            guard let shouldShowSuggestions =  searchEngines?.shouldShowSearchSuggestions else { return 0 }
            return shouldShowSuggestions && !searchQuery.looksLikeAURL() && !tabType.isPrivate ? min(suggestions.count, SearchViewControllerUX.maxSearchSuggestions) : 0
        case .bookmarksAndHistory:
            return data.count
        case .findInPage:
            if let sd = searchDelegate, sd.searchViewControllerAllowFindInPage() {
                return 1
            }
            return 0
        }
    }

    func numberOfSections(in tableView: UITableView) -> Int {
        return availableSections.count
    }

    func tableView(_ tableView: UITableView, didHighlightRowAt indexPath: IndexPath) {
        guard let section = availableSections[safe: indexPath.section] else {
            return
        }

        if section == .bookmarksAndHistory {
            let suggestion = data[indexPath.item]
            searchDelegate?.searchViewController(self, didHighlightText: suggestion.url, search: false)
        }
    }
}

// MARK: - KeyboardHelperDelegate

extension SearchViewController: KeyboardHelperDelegate {
    func keyboardHelper(_ keyboardHelper: KeyboardHelper, keyboardWillShowWithState state: KeyboardState) {
        animateSearchEnginesWithKeyboard(state)
    }

    func keyboardHelper(_ keyboardHelper: KeyboardHelper, keyboardWillHideWithState state: KeyboardState) {
        animateSearchEnginesWithKeyboard(state)
    }
}

// MARK: - KeyCommands

extension SearchViewController {
    func handleKeyCommands(sender: UIKeyCommand) {
        let initialSection = SearchListSection.bookmarksAndHistory.rawValue
        
        guard let current = tableView.indexPathForSelectedRow else {
            let numberOfRows = tableView(tableView, numberOfRowsInSection: initialSection)
            if sender.input == UIKeyCommand.inputDownArrow, numberOfRows > 0 {
                let next = IndexPath(item: 0, section: initialSection)
                self.tableView(tableView, didHighlightRowAt: next)
                tableView.selectRow(at: next, animated: false, scrollPosition: .top)
            }
            return
        }

        let nextSection: Int
        let nextItem: Int
        guard let input = sender.input else { return }
        
        switch input {
        case UIKeyCommand.inputUpArrow:
            // we're going down, we should check if we've reached the first item in this section.
            if current.item == 0 {
                // We have, so check if we can decrement the section.
                if current.section == initialSection {
                    // We've reached the first item in the first section.
                    searchDelegate?.searchViewController(self, didHighlightText: searchQuery, search: false)
                    return
                } else {
                    nextSection = current.section - 1
                    nextItem = tableView(tableView, numberOfRowsInSection: nextSection) - 1
                }
            } else {
                nextSection = current.section
                nextItem = current.item - 1
            }
        case UIKeyCommand.inputDownArrow:
            let currentSectionItemsCount = tableView(tableView, numberOfRowsInSection: current.section)
            if current.item == currentSectionItemsCount - 1 {
                if current.section == tableView.numberOfSections - 1 {
                    // We've reached the last item in the last section
                    return
                } else {
                    // We can go to the next section.
                    nextSection = current.section + 1
                    nextItem = 0
                }
            } else {
                nextSection = current.section
                nextItem = current.item + 1
            }
        default:
            return
        }
        
        guard nextItem >= 0 else { return }
        
        let next = IndexPath(item: nextItem, section: nextSection)
        tableView(tableView, didHighlightRowAt: next)
        tableView.selectRow(at: next, animated: false, scrollPosition: .middle)
    }
}

extension SearchViewController {
    func onSuggestionLongPressed(_ gestureRecognizer: UILongPressGestureRecognizer) {
        if gestureRecognizer.state == .began {
            let location = gestureRecognizer.location(in: self.tableView)
            if let indexPath = tableView.indexPathForRow(at: location),
               let section = availableSections[safe: indexPath.section],
               let suggestion = suggestions[safe: indexPath.row],
               section == .searchSuggestions {
                searchDelegate?.searchViewController(self, didLongPressSuggestion: suggestion)
            }
        }
    }
}

// MARK: - Private Extension - Class

/**
 * Private extension containing string operations specific to this view controller
 */
private extension String {
    func looksLikeAURL() -> Bool {
        // The assumption here is that if the user is typing in a forward slash and there are no spaces
        // involved, it's going to be a URL. If we type a space, any url would be invalid.
        // See https://bugzilla.mozilla.org/show_bug.cgi?id=1192155 for additional details.
        return self.contains("/") && !self.contains(" ")
    }
}

/**
 * UIScrollView that prevents buttons from interfering with scroll.
 */
private class ButtonScrollView: UIScrollView {
    fileprivate override func touchesShouldCancel(in view: UIView) -> Bool {
        return true
    }
}
