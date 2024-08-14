// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Data
import Favicon
import Preferences
import Shared
import Storage
import Then
import UIKit

// MARK: - SearchViewControllerDelegate

protocol SearchViewControllerDelegate: AnyObject {
  func searchViewController(
    _ searchViewController: SearchViewController,
    didSubmit query: String,
    braveSearchPromotion: Bool
  )
  func searchViewController(
    _ searchViewController: SearchViewController,
    didSubmitAIChat query: String
  )
  func searchViewController(_ searchViewController: SearchViewController, didSelectURL url: URL)
  func searchViewController(
    _ searchViewController: SearchViewController,
    didSelectOpenTab tabInfo: (id: UUID?, url: URL)
  )
  func searchViewController(
    _ searchViewController: SearchViewController,
    didLongPressSuggestion suggestion: String
  )
  func presentSearchSettingsController()
  func searchViewController(
    _ searchViewController: SearchViewController,
    didHighlightText text: String,
    search: Bool
  )
  func searchViewController(
    _ searchViewController: SearchViewController,
    shouldFindInPage query: String
  )
  func searchViewControllerAllowFindInPage() -> Bool
}

// MARK: - SearchViewController

public class SearchViewController: SiteTableViewController, LoaderListener {

  // MARK: SearchViewControllerUX

  private struct SearchViewControllerUX {
    static let engineButtonHeight: Float = 44
    static let engineButtonWidth = engineButtonHeight * 1.4

    static let searchImageWidth: Float = 24
    static let searchButtonMargin: CGFloat = 8

    static let faviconSize: CGFloat = 29
    static let iconBorderColor = UIColor(white: 0, alpha: 0.1)
    static let iconBorderWidth: CGFloat = 0.5
  }

  // MARK: Properties

  // Views for displaying the bottom scrollable search engine list. searchEngineScrollView is the
  // scrollable container; searchEngineScrollViewContent contains the actual set of search engine buttons.
  private let searchEngineScrollView = ButtonScrollView().then { scrollView in
    scrollView.decelerationRate = .fast
    scrollView.showsVerticalScrollIndicator = false
    scrollView.showsHorizontalScrollIndicator = false
    scrollView.clipsToBounds = false
    let border = UIView.separatorLine
    scrollView.addSubview(border)
    border.snp.makeConstraints {
      $0.bottom.equalTo(scrollView.snp.top)
      $0.leading.trailing.equalTo(scrollView.frameLayoutGuide)
    }
  }
  private let backgroundView = UIVisualEffectView(
    effect: UIBlurEffect(style: .systemUltraThinMaterial)
  )
  private let searchEngineScrollViewContent = UIView()

  private lazy var suggestionLongPressGesture = UILongPressGestureRecognizer(
    target: self,
    action: #selector(onSuggestionLongPressed(_:))
  )

  var searchDelegate: SearchViewControllerDelegate?
  var isUsingBottomBar: Bool = false {
    didSet {
      layoutSearchEngineScrollView()
    }
  }

  let dataSource: SearchSuggestionDataSource
  public static var userAgent: String?
  private var browserColors: any BrowserColors

  // MARK: Lifecycle

  init(with dataSource: SearchSuggestionDataSource, browserColors: some BrowserColors) {
    self.dataSource = dataSource
    self.browserColors = browserColors

    super.init(nibName: nil, bundle: nil)

    dataSource.delegate = self
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  deinit {
    NotificationCenter.default.removeObserver(self, name: .dynamicFontChanged, object: nil)
  }

  public override func viewSafeAreaInsetsDidChange() {
    super.viewSafeAreaInsetsDidChange()
    layoutSearchEngineScrollView()
  }

  override public func viewDidLoad() {
    super.viewDidLoad()

    view.insertSubview(backgroundView, belowSubview: tableView)

    backgroundView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    // Have to apply a custom alpha here because UIVisualEffectView blurs come with their own tint
    backgroundView.contentView.backgroundColor = browserColors.containerFrostedGlass
      .withAlphaComponent(0.8)
    searchEngineScrollView.backgroundColor = browserColors.containerBackground

    setupSearchEngineScrollViewIfNeeded()

    KeyboardHelper.defaultHelper.addDelegate(self)

    tableView.do {
      $0.keyboardDismissMode = .interactive
      $0.separatorStyle = .none
      $0.sectionHeaderTopPadding = 5
      $0.backgroundColor = .clear
      $0.addGestureRecognizer(suggestionLongPressGesture)
      $0.register(
        SearchSuggestionPromptCell.self,
        forCellReuseIdentifier: SearchSuggestionPromptCell.identifier
      )
      $0.register(SuggestionCell.self, forCellReuseIdentifier: SuggestionCell.identifier)
      $0.register(
        BraveSearchPromotionCell.self,
        forCellReuseIdentifier: BraveSearchPromotionCell.identifier
      )
      $0.register(UITableViewCell.self, forCellReuseIdentifier: "default")
    }
    NotificationCenter.default.addObserver(
      self,
      selector: #selector(dynamicFontChanged),
      name: .dynamicFontChanged,
      object: nil
    )
  }

  override public func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    reloadSearchEngines()
    reloadData()
  }

  override public func viewWillTransition(
    to size: CGSize,
    with coordinator: UIViewControllerTransitionCoordinator
  ) {
    super.viewWillTransition(to: size, with: coordinator)
    // The height of the suggestions row may change, so call reloadData() to recalculate cell heights.
    coordinator.animate(
      alongsideTransition: { _ in
        self.tableView.reloadData()
      },
      completion: nil
    )
  }

  private func animateSearchEnginesWithKeyboard(_ keyboardState: KeyboardState) {
    layoutSearchEngineScrollView()

    UIViewPropertyAnimator(
      duration: keyboardState.animationDuration,
      curve: keyboardState.animationCurve
    ) {
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
    if !dataSource.hasQuickSearchEngines { return }

    view.addSubview(searchEngineScrollView)
    searchEngineScrollView.addSubview(searchEngineScrollViewContent)

    layoutSearchEngineScrollView()
    layoutTable()

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
    if !dataSource.hasQuickSearchEngines { return }

    let keyboardHeight =
      KeyboardHelper.defaultHelper.currentState?.intersectionHeightForView(view) ?? 0

    searchEngineScrollView.snp.remakeConstraints { make in
      make.leading.trailing.equalTo(view)
      if isUsingBottomBar {
        let offset: CGFloat
        if keyboardHeight > 0 {
          offset =
            keyboardHeight - abs(additionalSafeAreaInsets.bottom - view.safeAreaInsets.bottom)
        } else {
          offset = 0
        }
        make.bottom.equalTo(view.safeArea.bottom).offset(-offset)
      } else {
        make.bottom.equalTo(view).offset(-keyboardHeight)
      }
    }
  }

  private func layoutSuggestionsOptInPrompt() {
    if dataSource.tabType.isPrivate
      || dataSource.searchEngines?.shouldShowSearchSuggestionsOptIn == false
    {
      reloadData()
      return
    }

    layoutTable()
  }

  private func layoutTable() {
    tableView.snp.remakeConstraints { make in
      make.top.equalTo(view.snp.top)
      make.leading.trailing.equalTo(view)
      make.bottom.equalTo(
        dataSource.hasQuickSearchEngines ? searchEngineScrollView.snp.top : self.view
      )
    }
  }

  override func reloadData() {
    dataSource.querySuggestClient()
  }

  func setupSearchEngineList() {
    // Cancel requests that are not executed
    dataSource.cancelPendingSuggestionsRequests()
    // Query and reload the table with new search suggestions.
    dataSource.querySuggestClient()
    dataSource.setupSearchClient()

    // Reload the footer list of search engines.
    reloadSearchEngines()
    layoutSuggestionsOptInPrompt()
  }

  func setSearchQuery(query: String, showSearchSuggestions: Bool = true) {
    dataSource.searchQuery = query
    // Do not query suggestions if the text entred is suspicious
    if showSearchSuggestions {
      dataSource.querySuggestClient()
    }
  }

  private func reloadSearchEngines() {
    searchEngineScrollViewContent.subviews.forEach { $0.removeFromSuperview() }
    var leftEdge = searchEngineScrollViewContent.snp.left

    // search settings icon
    let searchButton = UIButton()
    searchButton.setImage(
      UIImage(named: "quickSearch", in: .module, compatibleWith: nil)!.template,
      for: []
    )
    searchButton.imageView?.contentMode = .center
    searchButton.backgroundColor = .clear
    searchButton.addTarget(self, action: #selector(didClickSearchButton), for: .touchUpInside)
    searchButton.accessibilityLabel = Strings.searchSettingsButtonTitle
    searchButton.tintColor = .braveBlurpleTint

    searchButton.imageView?.snp.makeConstraints { make in
      make.width.height.equalTo(SearchViewControllerUX.searchImageWidth)
      return
    }

    searchEngineScrollViewContent.addSubview(searchButton)
    searchButton.snp.makeConstraints { make in
      make.size.equalTo(SearchViewControllerUX.faviconSize)
      // offset the left edge to align with search results
      make.left.equalTo(leftEdge).offset(SearchViewControllerUX.searchButtonMargin * 2)
      make.top.equalTo(searchEngineScrollViewContent).offset(
        SearchViewControllerUX.searchButtonMargin
      )
      make.bottom.equalTo(searchEngineScrollViewContent).offset(
        -SearchViewControllerUX.searchButtonMargin
      )
    }

    // search engines
    leftEdge = searchButton.snp.right
    for engine in dataSource.quickSearchEngines {
      let engineButton = UIButton()
      engineButton.setImage(engine.image, for: [])
      engineButton.imageView?.contentMode = .scaleAspectFit
      engineButton.backgroundColor = .clear
      engineButton.addTarget(self, action: #selector(didSelectEngine), for: .touchUpInside)
      engineButton.accessibilityLabel = String(
        format: Strings.searchEngineFormatText,
        engine.shortName
      )

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

        if engine === dataSource.searchEngines?.quickSearchEngines.last {
          make.right.equalTo(searchEngineScrollViewContent)
        }
      }
      leftEdge = engineButton.snp.right
    }
  }

  public func loader(dataLoaded data: [Site]) {
    self.data = Array(data.prefix(5))
    tableView.reloadData()
  }

  private func isBraveSearchPrompt(for indexPath: IndexPath) -> Bool {
    guard let section = dataSource.availableSections[safe: indexPath.section] else {
      return false
    }

    switch section {
    case .searchSuggestionsOptIn:
      return indexPath.row == 1 && dataSource.braveSearchPromotionAvailable
    case .searchSuggestions:
      let index =
        traitCollection.horizontalSizeClass == .regular && UIDevice.current.orientation.isPortrait
        ? 4 : 2
      switch dataSource.suggestions.count {
      case 0...index:
        return indexPath.row == dataSource.suggestions.count
          && dataSource.braveSearchPromotionAvailable
      default:
        return indexPath.row == index && dataSource.braveSearchPromotionAvailable
      }
    default:
      return false
    }
  }

  private func changeBraveSearchPromotionState() {
    // The promotion state will changed "maybeLaterUpcomingSession" next launch
    if Preferences.BraveSearch.braveSearchPromotionCompletionState.value
      != BraveSearchPromotionState.maybeLaterSameSession.rawValue
    {
      Preferences.BraveSearch.braveSearchPromotionCompletionState.value += 1
    }
  }

  private func submitSeachTemplateQuery(isBraveSearchPromotion: Bool) {
    if !dataSource.tabType.isPrivate {
      RecentSearch.addItem(type: .text, text: dataSource.searchQuery, websiteUrl: nil)
    }
    searchDelegate?.searchViewController(
      self,
      didSubmit: dataSource.searchQuery,
      braveSearchPromotion: isBraveSearchPromotion
    )
  }

  private func submitSearchQueryToAIChat() {
    searchDelegate?.searchViewController(
      self,
      didSubmitAIChat: dataSource.searchQuery
    )
  }

  // MARK: Actions

  @objc func didSelectEngine(_ sender: UIButton) {
    // The UIButtons are the same cardinality and order as the array of quick search engines.
    // Subtract 1 from index to account for magnifying glass accessory.
    guard let index = searchEngineScrollViewContent.subviews.firstIndex(of: sender) else {
      assertionFailure()
      return
    }

    let engine = dataSource.quickSearchEngines[index - 1]
    let localSearchQuery = dataSource.searchQuery.lowercased()
    guard let url = engine.searchURLForQuery(localSearchQuery) else {
      assertionFailure()
      return
    }

    if !dataSource.tabType.isPrivate {
      RecentSearch.addItem(type: .website, text: localSearchQuery, websiteUrl: url.absoluteString)
    }
    searchDelegate?.searchViewController(self, didSelectURL: url)
  }

  @objc func didClickSearchButton() {
    self.searchDelegate?.presentSearchSettingsController()
  }

  // MARK: UITableViewDelegate, UITableViewDataSource

  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    guard let section = dataSource.availableSections[safe: indexPath.section] else { return }

    switch section {
    case .quickBar:
      submitSeachTemplateQuery(isBraveSearchPromotion: false)
    case .aiChat:
      submitSearchQueryToAIChat()
    case .searchSuggestionsOptIn: return
    case .searchSuggestions:
      if !isBraveSearchPrompt(for: indexPath) {
        // Assume that only the default search engine can provide search suggestions.
        let engine = dataSource.searchEngines?.defaultEngine(
          forType: dataSource.tabType == .private ? .privateMode : .standard
        )
        let suggestion = dataSource.suggestions[indexPath.row]

        var url = URIFixup.getURL(suggestion)
        if url == nil {
          url = engine?.searchURLForQuery(suggestion)
        }

        if let url = url {
          if !dataSource.tabType.isPrivate {
            RecentSearch.addItem(type: .website, text: suggestion, websiteUrl: url.absoluteString)
          }
          searchDelegate?.searchViewController(self, didSelectURL: url)
        }
      }
    case .openTabsAndHistoryAndBookmarks:
      let site = data[indexPath.row]
      if let url = URL(string: site.url) {
        if site.siteType == .tab {
          var tabId: UUID?
          if let siteId = site.tabID {
            tabId = UUID(uuidString: siteId)
          }
          searchDelegate?.searchViewController(self, didSelectOpenTab: (tabId, url))
        } else {
          searchDelegate?.searchViewController(self, didSelectURL: url)
        }
      }
    case .findInPage:
      let localSearchQuery = dataSource.searchQuery.lowercased()
      searchDelegate?.searchViewController(self, shouldFindInPage: localSearchQuery)
    }
  }

  override public func tableView(
    _ tableView: UITableView,
    heightForRowAt indexPath: IndexPath
  ) -> CGFloat {
    if let currentSection = dataSource.availableSections[safe: indexPath.section] {
      switch currentSection {
      case .quickBar, .aiChat, .openTabsAndHistoryAndBookmarks, .findInPage:
        return super.tableView(tableView, heightForRowAt: indexPath)
      case .searchSuggestionsOptIn:
        return isBraveSearchPrompt(for: indexPath) ? UITableView.automaticDimension : 100.0
      case .searchSuggestions:
        return isBraveSearchPrompt(for: indexPath) ? UITableView.automaticDimension : 44.0
      }
    }

    return 0
  }

  func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
    guard let searchSection = dataSource.availableSections[safe: section] else { return nil }

    switch searchSection {
    case .quickBar, .aiChat: return nil
    case .searchSuggestionsOptIn: return nil
    case .searchSuggestions:
      if let defaultSearchEngine = dataSource.searchEngines?.defaultEngine(
        forType: dataSource.tabType == .private ? .privateMode : .standard
      ) {
        if defaultSearchEngine.shortName.contains(
          Strings.searchSuggestionSectionTitleNoSearchFormat
        ) || defaultSearchEngine.shortName.lowercased().contains("search") {
          return defaultSearchEngine.displayName
        }
        return String(
          format: Strings.searchSuggestionSectionTitleFormat,
          defaultSearchEngine.displayName
        )
      }
      return Strings.searchSuggestionsSectionHeader
    case .openTabsAndHistoryAndBookmarks: return Strings.searchHistorySectionHeader
    case .findInPage: return Strings.findOnPageSectionHeader
    }
  }

  func tableView(
    _ tableView: UITableView,
    willDisplayHeaderView view: UIView,
    forSection section: Int
  ) {
    if let headerView = view as? UITableViewHeaderFooterView {
      headerView.textLabel?.textColor = .bravePrimary
    }
  }

  override public func tableView(
    _ tableView: UITableView,
    heightForHeaderInSection section: Int
  ) -> CGFloat {
    guard let searchSection = dataSource.availableSections[safe: section] else { return 0 }
    let headerHeight: CGFloat = 22

    switch searchSection {
    case .quickBar, .aiChat, .searchSuggestionsOptIn:
      return 0.0
    case .searchSuggestions:
      return dataSource.suggestions.isEmpty ? 0 : headerHeight * 2.0
    case .openTabsAndHistoryAndBookmarks:
      // Check for History Bookmarks Open Tabs suggestions
      // Show Browser Suggestions Preference effects all the modes
      if !Preferences.Search.showBrowserSuggestions.value {
        return 0
      }

      // Private Browsing Mode (PBM) should *not* show items from normal mode History etc
      // when search suggestions is not enabled
      if Preferences.Privacy.privateBrowsingOnly.value,
        dataSource.searchEngines?.shouldShowSearchSuggestions == false
      {
        return 0
      }

      return data.isEmpty ? 0 : 2.0 * headerHeight
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
    guard let searchSection = dataSource.availableSections[safe: section] else { return 0 }
    let footerHeight: CGFloat = 10.0

    switch searchSection {
    case .quickBar, .searchSuggestions, .findInPage, .openTabsAndHistoryAndBookmarks:
      return CGFloat.leastNormalMagnitude
    case .searchSuggestionsOptIn:
      return footerHeight
    case .aiChat:
      return footerHeight
    }
  }

  override public func tableView(
    _ tableView: UITableView,
    cellForRowAt indexPath: IndexPath
  ) -> UITableViewCell {
    func createSearchSuggestionPromotionCell() -> UITableViewCell {
      let cell = tableView.dequeueReusableCell(
        withIdentifier: BraveSearchPromotionCell.identifier,
        for: indexPath
      )
      if let promotionSearchCell = cell as? BraveSearchPromotionCell {
        promotionSearchCell.trySearchEngineTapped = { [weak self] in
          self?.submitSeachTemplateQuery(isBraveSearchPromotion: true)
        }

        promotionSearchCell.dismissTapped = { [weak self] in
          self?.changeBraveSearchPromotionState()
          tableView.reloadData()
        }
      }

      return cell
    }

    guard let section = dataSource.availableSections[safe: indexPath.section] else {
      return UITableViewCell()
    }

    switch section {
    case .quickBar:
      let cell = TwoLineTableViewCell().then {
        $0.textLabel?.text = dataSource.searchQuery
        $0.textLabel?.textColor = .bravePrimary
        $0.imageView?.image = UIImage(
          named: "search_bar_find_in_page_icon",
          in: .module,
          compatibleWith: nil
        )?.withRenderingMode(.alwaysTemplate)
        $0.imageView?.tintColor = browserColors.iconDefault
        $0.imageView?.contentMode = .center
        $0.backgroundColor = .clear
      }

      return cell
    case .aiChat:
      let cell = TwoLineTableViewCell().then {
        $0.textLabel?.text =
          "\(dataSource.searchQuery) - \(Strings.AIChat.askLeoSearchSuggestionTitle)"
        $0.textLabel?.textColor = .bravePrimary
        $0.imageView?.image = UIImage(named: "aichat-avatar", in: .module, compatibleWith: nil)
        $0.imageView?.tintColor = browserColors.iconDefault
        $0.imageView?.contentMode = .center
        $0.backgroundColor = .clear
      }

      return cell
    case .searchSuggestionsOptIn:
      var cell: UITableViewCell?

      if isBraveSearchPrompt(for: indexPath) {
        cell = createSearchSuggestionPromotionCell()
      } else {
        cell = tableView.dequeueReusableCell(
          withIdentifier: SearchSuggestionPromptCell.identifier,
          for: indexPath
        )
        if let promptCell = cell as? SearchSuggestionPromptCell {
          promptCell.selectionStyle = .none
          promptCell.onOptionSelected = { [weak self] option in
            guard let self = self else { return }

            self.dataSource.searchEngines?.shouldShowSearchSuggestions = option
            self.dataSource.searchEngines?.shouldShowSearchSuggestionsOptIn = false

            if option {
              self.dataSource.querySuggestClient()
            }
            self.layoutSuggestionsOptInPrompt()
            self.reloadSearchEngines()
            self.tableView.reloadData()
          }
        }
      }

      guard let tableViewCell = cell else { return UITableViewCell() }
      tableViewCell.separatorInset = .zero

      return tableViewCell
    case .searchSuggestions:
      var cell: UITableViewCell?

      if isBraveSearchPrompt(for: indexPath) {
        cell = createSearchSuggestionPromotionCell()
      } else {
        cell = tableView.dequeueReusableCell(
          withIdentifier: SuggestionCell.identifier,
          for: indexPath
        )

        if let suggestionCell = cell as? SuggestionCell,
          let suggestion = dataSource.suggestions[safe: indexPath.row]
        {
          suggestionCell.setTitle(suggestion)
          suggestionCell.separatorInset = UIEdgeInsets(
            top: 0.0,
            left: view.bounds.width,
            bottom: 0.0,
            right: -view.bounds.width
          )
          suggestionCell.openButtonActionHandler = { [weak self] in
            guard let self = self else { return }

            self.searchDelegate?.searchViewController(self, didLongPressSuggestion: suggestion)
          }
        }
      }

      guard let tableViewCell = cell else { return UITableViewCell() }
      tableViewCell.separatorInset = .zero

      return tableViewCell
    case .openTabsAndHistoryAndBookmarks:
      let cell = super.tableView(tableView, cellForRowAt: indexPath)
      let site = data[indexPath.row]

      let detailTextForTabSuggestions = NSMutableAttributedString()

      detailTextForTabSuggestions.append(
        NSAttributedString(
          string: Strings.searchSuggestionOpenTabActionTitle,
          attributes: [
            .font: DynamicFontHelper.defaultHelper.smallSizeBoldWeightAS,
            .foregroundColor: browserColors.textSecondary,
          ]
        )
      )

      detailTextForTabSuggestions.append(
        NSAttributedString(
          string: " · \(site.url)",
          attributes: [
            .font: DynamicFontHelper.defaultHelper.smallSizeRegularWeightAS,
            .foregroundColor: browserColors.textSecondary,
          ]
        )
      )

      if let cell = cell as? TwoLineTableViewCell {
        cell.textLabel?.textColor = browserColors.textPrimary
        if site.siteType == .tab {
          cell.setLines(
            site.title,
            detailText: nil,
            detailAttributedText: detailTextForTabSuggestions
          )
        } else {
          cell.setLines(site.title, detailText: site.url)
        }
        cell.setRightBadge(site.siteType.icon?.template ?? nil)
        cell.accessoryView?.tintColor = browserColors.iconDefault

        cell.imageView?.contentMode = .scaleAspectFit
        cell.imageView?.layer.borderColor = SearchViewControllerUX.iconBorderColor.cgColor
        cell.imageView?.layer.borderWidth = SearchViewControllerUX.iconBorderWidth
        cell.imageView?.loadFavicon(
          for: site.tileURL,
          isPrivateBrowsing: dataSource.tabType.isPrivate
        )
        cell.backgroundColor = .clear
      }

      return cell
    case .findInPage:
      let cell = tableView.dequeueReusableCell(withIdentifier: "default", for: indexPath)
      cell.textLabel?.text = String(format: Strings.findInPageFormat, dataSource.searchQuery)
      cell.textLabel?.textColor = browserColors.textPrimary
      cell.textLabel?.numberOfLines = 2
      cell.textLabel?.font = .systemFont(ofSize: 15.0)
      cell.textLabel?.lineBreakMode = .byWordWrapping
      cell.textLabel?.textAlignment = .left
      cell.backgroundColor = .clear

      return cell
    }
  }

  override public func tableView(
    _ tableView: UITableView,
    numberOfRowsInSection section: Int
  ) -> Int {
    guard let section = dataSource.availableSections[safe: section] else {
      return 0
    }

    switch section {
    case .quickBar, .aiChat:
      return 1
    case .searchSuggestionsOptIn:
      return dataSource.braveSearchPromotionAvailable ? 2 : 1
    case .searchSuggestions:
      guard let shouldShowSuggestions = dataSource.searchEngines?.shouldShowSearchSuggestions else {
        return 0
      }

      var searchSuggestionsCount = min(
        dataSource.suggestions.count,
        dataSource.maxSearchSuggestions
      )
      if dataSource.braveSearchPromotionAvailable {
        searchSuggestionsCount += 1
      }

      return shouldShowSuggestions && !dataSource.searchQuery.looksLikeAURL()
        && !dataSource.tabType.isPrivate ? searchSuggestionsCount : 0
    case .openTabsAndHistoryAndBookmarks:
      // Check for History Bookmarks Open Tabs suggestions
      // Show Browser Suggestions Preference effects all the modes
      if !Preferences.Search.showBrowserSuggestions.value {
        return 0
      }

      // Private Browsing Mode (PBM) should *not* show items from normal mode History etc
      // when search suggestions is not enabled
      if Preferences.Privacy.privateBrowsingOnly.value,
        dataSource.searchEngines?.shouldShowSearchSuggestions == false
      {
        return 0
      }

      return data.count
    case .findInPage:
      if let sd = searchDelegate, sd.searchViewControllerAllowFindInPage() {
        return 1
      }
      return 0
    }
  }

  func numberOfSections(in tableView: UITableView) -> Int {
    return dataSource.availableSections.count
  }
}

// MARK: - SearchSuggestionDataSourceDelegate

extension SearchViewController: SearchSuggestionDataSourceDelegate {
  func searchSuggestionDataSourceReloaded() {
    tableView.reloadData()
  }
}

// MARK: - KeyboardHelperDelegate

extension SearchViewController: KeyboardHelperDelegate {
  public func keyboardHelper(
    _ keyboardHelper: KeyboardHelper,
    keyboardWillShowWithState state: KeyboardState
  ) {
    animateSearchEnginesWithKeyboard(state)
  }

  public func keyboardHelper(
    _ keyboardHelper: KeyboardHelper,
    keyboardWillHideWithState state: KeyboardState
  ) {
    animateSearchEnginesWithKeyboard(state)
  }
}

// MARK: - UILongPressGestureRecognizer

extension SearchViewController {
  func onSuggestionLongPressed(_ gestureRecognizer: UILongPressGestureRecognizer) {
    if gestureRecognizer.state == .began {
      let location = gestureRecognizer.location(in: self.tableView)
      if let indexPath = tableView.indexPathForRow(at: location),
        let section = dataSource.availableSections[safe: indexPath.section],
        let suggestion = dataSource.suggestions[safe: indexPath.row],
        section == .searchSuggestions
      {
        searchDelegate?.searchViewController(self, didLongPressSuggestion: suggestion)
      }
    }
  }
}

// MARK: - ButtonScrollView

/// UIScrollView that prevents buttons from interfering with scroll.
private class ButtonScrollView: UIScrollView {
  fileprivate override func touchesShouldCancel(in view: UIView) -> Bool {
    return true
  }
}
