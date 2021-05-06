/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared
import Storage
import BraveShared

// MARK: - SearchViewControllerDelegate

protocol SearchViewControllerDelegate: class {
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
    }

    // MARK: SearchListSection
    private enum SearchListSection: Int, CaseIterable {
        case searchSuggestions
        case findInPage
        case bookmarksAndHistory
    }

    // MARK: Properties
    
    private let tabType: TabType
    private var suggestClient: SearchSuggestClient?

    // Views for displaying the bottom scrollable search engine list. searchEngineScrollView is the
    // scrollable container; searchEngineScrollViewContent contains the actual set of search engine buttons.
    private let searchEngineScrollView = ButtonScrollView().then { scrollView in
        scrollView.decelerationRate = UIScrollView.DecelerationRate.fast
        scrollView.showsVerticalScrollIndicator = false
        scrollView.showsHorizontalScrollIndicator = false
        scrollView.clipsToBounds = false
        let border = UIView.separatorLine
        scrollView.addSubview(border)
        border.snp.makeConstraints {
            $0.bottom.equalTo(scrollView.snp.top)
            $0.leading.trailing.equalToSuperview()
        }
    }
    private let searchEngineScrollViewContent = UIView().then {
        $0.backgroundColor = .braveBackground
    }
    
    private lazy var bookmarkedBadge: UIImage = {
        return #imageLiteral(resourceName: "bookmarked_passive")
    }()

    // Cell for the suggestion flow layout. Since heightForHeaderInSection is called *before*
    // cellForRowAtIndexPath, we create the cell to find its height before it's added to the table.
    private let suggestionCell = SuggestionCell(style: .default, reuseIdentifier: nil)
    private var suggestionPrompt: SearchSuggestionPromptView?
    
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

        suggestionCell.delegate = self

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

        UIView.animate(withDuration: keyboardState.animationDuration, animations: {
            UIView.setAnimationCurve(keyboardState.animationCurve)
            self.view.layoutIfNeeded()
        })
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
                make.left.equalTo(searchEngineScrollView).priority(.required)
            } else {
                make.left.greaterThanOrEqualTo(searchEngineScrollView).priority(.required)
            }
            make.bottom.right.top.equalTo(searchEngineScrollView)
        }
    }

    private func layoutSearchEngineScrollView() {
        if !hasQuickSearchEngines { return }
        
        let keyboardHeight = KeyboardHelper.defaultHelper.currentState?.intersectionHeightForView(view) ?? 0
        searchEngineScrollView.snp.remakeConstraints { make in
            make.left.right.equalTo(view)
            make.bottom.equalTo(view).offset(-keyboardHeight)
            make.height.equalTo(SearchViewControllerUX.engineButtonHeight)
        }
    }
    
    private func layoutSuggestionsOptInPrompt() {
        if tabType.isPrivate || searchEngines?.shouldShowSearchSuggestionsOptIn == false {
            // Make sure any pending layouts are drawn so they don't get coupled
            // with the "slide up" animation below.
            view.layoutIfNeeded()
            
            // Set the prompt to nil so layoutTable() aligns the top of the table
            // to the top of the view. We still need a reference to the prompt so
            // we can remove it from the controller after the animation is done.
            let prompt = suggestionPrompt
            suggestionPrompt = nil
            layoutTable()
            
            UIView.animate(withDuration: 0.2,
                           animations: {
                            self.view.layoutIfNeeded()
                            prompt?.alpha = 0
            },
                           completion: { _ in
                            prompt?.removeFromSuperview()
                            return
            })
            return
        }
        
        let prompt = SearchSuggestionPromptView() { [weak self] option in
            guard let self = self else { return }
            
            self.searchEngines?.shouldShowSearchSuggestions = option
            self.searchEngines?.shouldShowSearchSuggestionsOptIn = false
            
            if option {
                self.querySuggestClient()
            }
            self.layoutSuggestionsOptInPrompt()
            self.reloadSearchEngines()
        }
        // Insert behind the tableView so the tableView slides on top of it
        // when the prompt is dismissed.
        view.addSubview(prompt)
        suggestionPrompt = prompt
        
        prompt.snp.makeConstraints { make in
            make.top.equalTo(self.view)
            make.leading.equalTo(self.view.safeAreaLayoutGuide.snp.leading)
            make.trailing.equalTo(self.view.safeAreaLayoutGuide.snp.trailing)
        }
        
        layoutTable()
    }

    private func layoutTable() {
        tableView.snp.remakeConstraints { make in
            make.top.equalTo(suggestionPrompt?.snp.bottom ?? view.snp.top)
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

        if searchQuery.isEmpty || searchEngines?.shouldShowSearchSuggestionsOptIn == true || searchQuery.looksLikeAURL() {
            suggestionCell.suggestions = []
            return
        }

        suggestClient?.query(searchQuery, callback: { [weak self] suggestions, error in
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
                self.suggestionCell.suggestions = suggestionList
            }

            // If there are no suggestions, just use whatever the user typed.
            if suggestions?.isEmpty ?? true {
                self.suggestionCell.suggestions = [self.searchQuery]
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

        guard let url = engine.searchURLForQuery(searchQuery) else {
            assertionFailure()
            return
        }

        searchDelegate?.searchViewController(self, didSelectURL: url)
    }

    @objc func didClickSearchButton() {
        self.searchDelegate?.presentSearchSettingsController()
    }
    
    // MARK: UITableViewDelegate, UITableViewDataSource
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        guard let section = SearchListSection(rawValue: indexPath.section) else { return }
        
        if section == SearchListSection.bookmarksAndHistory {
            let site = data[indexPath.row]
            if let url = URL(string: site.url) {
                searchDelegate?.searchViewController(self, didSelectURL: url)
            }
        } else if section == SearchListSection.findInPage {
            searchDelegate?.searchViewController(self, shouldFindInPage: searchQuery)
        }
    }

    override func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
        if let currentSection = SearchListSection(rawValue: indexPath.section) {
            switch currentSection {
            case .searchSuggestions:
                // heightForRowAtIndexPath is called *before* the cell is created, so to get the height,
                // force a layout pass first.
                suggestionCell.layoutIfNeeded()
                return suggestionCell.frame.height
            default:
                return super.tableView(tableView, heightForRowAt: indexPath)
            }
        }

        return 0
    }
    
    func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        guard let searchSection = SearchListSection(rawValue: section) else { return nil }
        
        switch searchSection {
            case .searchSuggestions: return nil
            case .findInPage: return Strings.findOnPageSectionHeader
            case .bookmarksAndHistory: return Strings.searchHistorySectionHeader
        }
    }

    override func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
        guard let searchSection = SearchListSection(rawValue: section) else { return 0 }
        let headerHeight: CGFloat = 22
        
        switch searchSection {
        case .findInPage:
            if let sd = searchDelegate, sd.searchViewControllerAllowFindInPage() {
                return headerHeight
            }
            return 0
        case .bookmarksAndHistory: return data.isEmpty ? 0 : headerHeight
        default: return 0
        }
    }

    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        switch SearchListSection(rawValue: indexPath.section)! {
        case .searchSuggestions:
            suggestionCell.imageView?.image = searchEngines?.defaultEngine().image
            suggestionCell.imageView?.isAccessibilityElement = true
            if let defaultSearchEngine = searchEngines?.defaultEngine() {
                suggestionCell.imageView?.accessibilityLabel = String(format: Strings.searchSuggestionFromFormatText, defaultSearchEngine.shortName)
            }
            return suggestionCell
            
        case .findInPage:
            let cell = TwoLineTableViewCell()
            cell.textLabel?.text = String(format: Strings.findInPageFormat, searchQuery)
            cell.imageView?.image = #imageLiteral(resourceName: "search_bar_find_in_page_icon")
            cell.imageView?.contentMode = .center
            
            return cell

        case .bookmarksAndHistory:
            let cell = super.tableView(tableView, cellForRowAt: indexPath)
            let site = data[indexPath.row]
            if let cell = cell as? TwoLineTableViewCell {
                let isBookmark = site.bookmarked ?? false
                cell.setLines(site.title, detailText: site.url)
                cell.setRightBadge(isBookmark ? self.bookmarkedBadge : nil)
                cell.imageView?.contentMode = .scaleAspectFit
                cell.imageView?.layer.borderColor = SearchViewControllerUX.iconBorderColor.cgColor
                cell.imageView?.layer.borderWidth = SearchViewControllerUX.iconBorderWidth
                cell.imageView?.image = UIImage()
                cell.imageView?.loadFavicon(for: site.tileURL)
            }
            return cell
        }
    }

    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        switch SearchListSection(rawValue: section)! {
        case .searchSuggestions:
            guard let shouldShowSuggestions =  searchEngines?.shouldShowSearchSuggestions else { return 0 }

            return shouldShowSuggestions && !searchQuery.looksLikeAURL() && !tabType.isPrivate ? 1 : 0
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
        return SearchListSection.allCases.count
    }

    func tableView(_ tableView: UITableView, didHighlightRowAt indexPath: IndexPath) {
        guard let section = SearchListSection(rawValue: indexPath.section) else {
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

// MARK: - SuggestionCellDelegate

extension SearchViewController: SuggestionCellDelegate {
    internal func suggestionCell(_ suggestionCell: SuggestionCell, didSelectSuggestion suggestion: String) {
        // Assume that only the default search engine can provide search suggestions.
        let engine = searchEngines?.defaultEngine()

        var url = URIFixup.getURL(suggestion)
        if url == nil {
            url = engine?.searchURLForQuery(suggestion)
        }

        if let url = url {
            searchDelegate?.searchViewController(self, didSelectURL: url)
        }
    }

    internal func suggestionCell(_ suggestionCell: SuggestionCell, didLongPressSuggestion suggestion: String) {
        searchDelegate?.searchViewController(self, didLongPressSuggestion: suggestion)
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
