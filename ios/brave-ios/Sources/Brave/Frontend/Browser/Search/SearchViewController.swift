// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Data
import DesignSystem
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

public class SearchViewController: UIViewController, LoaderListener {

  // MARK: SearchViewControllerUX

  private struct SearchViewControllerUX {
    static let engineButtonHeight: Float = 44
    static let engineButtonWidth = engineButtonHeight * 1.4

    static let searchImageWidth: Float = 24
    static let searchButtonMargin: CGFloat = 8

    static let faviconSize: CGFloat = 29
    static let separatorSize = CGSize(width: 1.0, height: 20.0)
  }

  // MARK: Properties

  // UI Properties
  lazy var collectionView: UICollectionView =
    UICollectionView(
      frame: .zero,
      collectionViewLayout: compositionalLayout
    )

  private let layoutConfig = UICollectionViewCompositionalLayoutConfiguration().then {
    $0.interSectionSpacing = 8.0
  }

  private lazy var compositionalLayout = UICollectionViewCompositionalLayout(
    sectionProvider: { sectionIndex, _ in
      let section = self.availableSections[sectionIndex]
      if section == .braveSearchPromotion {
        return self.braveSearchPromotionLayoutSection()
      }
      return self.searchLayoutSection(
        headerEnabled: section != .searchSuggestions,
        footerEnabled: section == .openTabsAndHistoryAndBookmarks
      )
    },
    configuration: layoutConfig
  ).then {
    $0.register(
      FavoritesSectionBackgroundView.self,
      forDecorationViewOfKind: "background_with_header"
    )
    $0.register(
      SearchSectionBackgroundView.self,
      forDecorationViewOfKind: "background_plain"
    )
  }
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

  weak var searchDelegate: SearchViewControllerDelegate?
  var profile: Profile! {
    didSet {
      collectionView.reloadData()
    }
  }
  var isUsingBottomBar: Bool = false {
    didSet {
      layoutSearchEngineScrollView()
    }
  }

  let dataSource: SearchSuggestionDataSource
  public static var userAgent: String?
  private var browserColors: any BrowserColors

  private var availableSections: [SearchSuggestionDataSource.SearchListSection] {
    var result = dataSource.searchSuggestionSections
    if let sd = searchDelegate, sd.searchViewControllerAllowFindInPage() {
      result.append(.findInPage)
    }
    if !Preferences.Search.showBrowserSuggestions.value {
    } else if Preferences.Privacy.privateBrowsingOnly.value
      && dataSource.searchEngines?.shouldShowSearchSuggestions == false
    {
    } else if !dataSource.siteData.isEmpty {
      result.append(.openTabsAndHistoryAndBookmarks)
    }
    return result
  }

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

    view.addSubview(backgroundView)
    view.addSubview(collectionView)

    backgroundView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    // Have to apply a custom alpha here because UIVisualEffectView blurs come with their own tint
    backgroundView.contentView.backgroundColor = browserColors.containerFrostedGlass
      .withAlphaComponent(0.8)
    searchEngineScrollView.backgroundColor = browserColors.containerBackground

    setupSearchEngineScrollViewIfNeeded()

    collectionView.do {
      $0.register(SearchSuggestionCell.self)
      $0.register(SearchFindInPageCell.self)
      $0.register(SearchOnYourDeviceCell.self)
      $0.register(
        SearchSectionHeaderView.self,
        forSupplementaryViewOfKind: UICollectionView.elementKindSectionHeader,
        withReuseIdentifier: "search_header"
      )
      $0.register(
        FavoritesRecentSearchFooterView.self,
        forSupplementaryViewOfKind: UICollectionView.elementKindSectionFooter,
        withReuseIdentifier: "in_your_device_footer"
      )
      $0.alwaysBounceVertical = true
      $0.backgroundColor = .clear
      $0.delegate = self
      $0.dataSource = self
      $0.keyboardDismissMode = .interactive
      $0.addGestureRecognizer(suggestionLongPressGesture)
    }

    KeyboardHelper.defaultHelper.addDelegate(self)

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
    reloadSearchData()
  }

  override public func viewWillTransition(
    to size: CGSize,
    with coordinator: UIViewControllerTransitionCoordinator
  ) {
    super.viewWillTransition(to: size, with: coordinator)
    // The height of the suggestions row may change, so call reloadData() to recalculate cell heights.
    coordinator.animate(
      alongsideTransition: { _ in
        self.collectionView.reloadData()
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
      //      reloadData()
      reloadSearchData()
    }
  }

  // MARK: Internal

  private func setupSearchEngineScrollViewIfNeeded() {
    guard dataSource.isAIChatAvailable || dataSource.hasQuickSearchEngines else { return }

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
    guard dataSource.isAIChatAvailable || dataSource.hasQuickSearchEngines else { return }

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
    if dataSource.isPrivate
      || dataSource.searchEngines?.shouldShowSearchSuggestionsOptIn == false
    {
      reloadSearchData()
      return
    }

    layoutTable()
  }

  private func layoutTable() {
    collectionView.snp.remakeConstraints { make in
      make.top.equalTo(view.snp.top)
      make.leading.trailing.equalTo(view)
      make.bottom.equalTo(searchEngineScrollView.snp.top)
    }
  }

  func reloadSearchData() {
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

    let leoButtonContainer = UIView().then {
      $0.layer.cornerRadius = 8.0
      $0.clipsToBounds = true
    }

    let leoButtonBackgroundView = GradientView(braveSystemName: .iconsActive)

    let leoButton = UIButton()
    leoButton.setImage(
      UIImage(braveSystemNamed: "leo.product.brave-leo")!.template,
      for: []
    )
    leoButton.imageView?.contentMode = .center
    leoButton.addTarget(self, action: #selector(didClickLeoButton), for: .touchUpInside)
    leoButton.accessibilityLabel = Strings.searchSettingsButtonTitle
    leoButton.tintColor = .white
    leoButton.imageView?.snp.makeConstraints { make in
      make.width.height.equalTo(SearchViewControllerUX.searchImageWidth)
      return
    }

    leoButtonContainer.addSubview(leoButtonBackgroundView)
    leoButtonContainer.addSubview(leoButton)

    leoButtonBackgroundView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    leoButton.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    searchEngineScrollViewContent.addSubview(leoButtonContainer)
    leoButtonContainer.snp.makeConstraints { make in
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

    leftEdge = leoButtonContainer.snp.right

    let separator = UIView().then {
      $0.backgroundColor = UIColor(braveSystemName: .materialSeparator)
    }

    searchEngineScrollView.addSubview(separator)
    separator.snp.makeConstraints { make in
      make.size.equalTo(SearchViewControllerUX.separatorSize)
      make.left.equalTo(leftEdge).offset(SearchViewControllerUX.searchButtonMargin * 2)
      make.centerY.equalTo(leoButtonBackgroundView)
    }

    // search engines
    leftEdge = separator.snp.right
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
      }
      leftEdge = engineButton.snp.right
    }

    let settingsButton = UIButton()
    settingsButton.setImage(
      UIImage(braveSystemNamed: "leo.settings")!.template,
      for: []
    )
    settingsButton.imageView?.contentMode = .center
    settingsButton.backgroundColor = .clear
    settingsButton.addTarget(self, action: #selector(didClickSettingsButton), for: .touchUpInside)
    settingsButton.accessibilityLabel = Strings.searchSettingsButtonTitle
    settingsButton.tintColor = UIColor(braveSystemName: .iconDefault)

    settingsButton.imageView?.snp.makeConstraints { make in
      make.width.height.equalTo(SearchViewControllerUX.searchImageWidth)
      return
    }

    searchEngineScrollViewContent.addSubview(settingsButton)
    settingsButton.snp.makeConstraints { make in
      make.size.equalTo(SearchViewControllerUX.faviconSize)
      // offset the left edge to align with search results
      make.left.equalTo(leftEdge).offset(
        SearchViewControllerUX.searchButtonMargin
      )
      make.top.equalTo(searchEngineScrollViewContent).offset(
        SearchViewControllerUX.searchButtonMargin
      )
      make.bottom.equalTo(searchEngineScrollViewContent).offset(
        -SearchViewControllerUX.searchButtonMargin
      )
      make.right.equalTo(searchEngineScrollViewContent).inset(
        SearchViewControllerUX.searchButtonMargin * 2
      )
    }
  }

  public func loader(dataLoaded data: [Site]) {
    dataSource.siteData = data
    collectionView.reloadData()
  }

  private func isBraveSearchPrompt(for indexPath: IndexPath) -> Bool {
    guard let section = availableSections[safe: indexPath.section] else {
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
    if !dataSource.isPrivate {
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

  private func searchLayoutSection(
    headerEnabled: Bool,
    footerEnabled: Bool
  ) -> NSCollectionLayoutSection {
    let itemSize = NSCollectionLayoutSize(
      widthDimension: .fractionalWidth(1),
      heightDimension: .absolute(52)
    )
    let item = NSCollectionLayoutItem(layoutSize: itemSize)
    let groupSize = NSCollectionLayoutSize(
      widthDimension: .fractionalWidth(1),
      heightDimension: .absolute(52)
    )
    item.contentInsets = .init(top: 8, leading: 14, bottom: 8, trailing: 14)
    let group = NSCollectionLayoutGroup.vertical(layoutSize: groupSize, subitems: [item])
    let section = NSCollectionLayoutSection(group: group)
    section.contentInsets = .init(top: 16, leading: 16, bottom: 16, trailing: 16)

    var supplementaryItems = [NSCollectionLayoutBoundarySupplementaryItem]()
    if headerEnabled {
      let headerItemSize = NSCollectionLayoutSize(
        widthDimension: .fractionalWidth(1),
        heightDimension: .absolute(50)
      )
      let headerItem = NSCollectionLayoutBoundarySupplementaryItem(
        layoutSize: headerItemSize,
        elementKind: UICollectionView.elementKindSectionHeader,
        alignment: .top
      )
      supplementaryItems.append(headerItem)

      let backgroundItem = NSCollectionLayoutDecorationItem.background(
        elementKind: "background_with_header"
      )
      backgroundItem.contentInsets = .init(top: 0, leading: 8, bottom: 0, trailing: 8)
      section.decorationItems = [backgroundItem]
    } else {
      let backgroundItem = NSCollectionLayoutDecorationItem.background(
        elementKind: "background_plain"
      )
      backgroundItem.contentInsets = .init(top: 0, leading: 8, bottom: 0, trailing: 8)
      section.decorationItems = [backgroundItem]
    }

    if footerEnabled {
      if dataSource.isSiteDataShowMoreAvailable {
        let footerItemSize = NSCollectionLayoutSize(
          widthDimension: .fractionalWidth(1),
          heightDimension: .estimated(30)
        )
        let footerItem = NSCollectionLayoutBoundarySupplementaryItem(
          layoutSize: footerItemSize,
          elementKind: UICollectionView.elementKindSectionFooter,
          alignment: .bottom
        )
        supplementaryItems.append(footerItem)
      }
    }

    section.boundarySupplementaryItems = supplementaryItems

    return section
  }

  private func braveSearchPromotionLayoutSection() -> NSCollectionLayoutSection {
    let itemSize = NSCollectionLayoutSize(
      widthDimension: .fractionalWidth(1),
      heightDimension: .fractionalHeight(1)
    )
    let item = NSCollectionLayoutItem(layoutSize: itemSize)
    let groupSize = NSCollectionLayoutSize(
      widthDimension: .fractionalWidth(1),
      heightDimension: .absolute(164)
    )
    item.contentInsets = .init(top: 8, leading: 14, bottom: 8, trailing: 14)
    let group = NSCollectionLayoutGroup.vertical(layoutSize: groupSize, subitems: [item])
    let section = NSCollectionLayoutSection(group: group)
    section.contentInsets = .init(top: 16, leading: 16, bottom: 16, trailing: 16)
    
    let backgroundItem = NSCollectionLayoutDecorationItem.background(
      elementKind: "background_plain"
    )
    backgroundItem.contentInsets = .init(top: 0, leading: 8, bottom: 0, trailing: 8)
    section.decorationItems = [backgroundItem]

    return section
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

    if !dataSource.isPrivate {
      RecentSearch.addItem(type: .website, text: localSearchQuery, websiteUrl: url.absoluteString)
    }
    searchDelegate?.searchViewController(self, didSelectURL: url)
  }

  @objc func didClickSettingsButton() {
    self.searchDelegate?.presentSearchSettingsController()
  }

  @objc func didClickLeoButton() {
    submitSearchQueryToAIChat()
  }

  @objc func onShowMorePressed() {
    dataSource.siteDataFetchLimit = 0
    collectionView.reloadData()
  }

  // MARK: UITableViewDelegate, UITableViewDataSource

  //
  //
  //
  //
  //  override public func tableView(
  //    _ tableView: UITableView,
  //    cellForRowAt indexPath: IndexPath
  //  ) -> UITableViewCell {
  //    func createSearchSuggestionPromotionCell() -> UITableViewCell {
  //      let cell = tableView.dequeueReusableCell(
  //        withIdentifier: BraveSearchPromotionCell.identifier,
  //        for: indexPath
  //      )
  //      if let promotionSearchCell = cell as? BraveSearchPromotionCell {
  //        promotionSearchCell.trySearchEngineTapped = { [weak self] in
  //          self?.submitSeachTemplateQuery(isBraveSearchPromotion: true)
  //        }
  //
  //        promotionSearchCell.dismissTapped = { [weak self] in
  //          self?.changeBraveSearchPromotionState()
  //          tableView.reloadData()
  //        }
  //      }
  //
  //      return cell
  //    }
  //
  //    guard let section = dataSource.availableSections[safe: indexPath.section] else {
  //      return UITableViewCell()
  //    }
  //
  //    switch section {
  //    case .searchSuggestionsOptIn:
  //      var cell: UITableViewCell?
  //
  //      if isBraveSearchPrompt(for: indexPath) {
  //        cell = createSearchSuggestionPromotionCell()
  //      } else {
  //        cell = tableView.dequeueReusableCell(
  //          withIdentifier: SearchSuggestionPromptCell.identifier,
  //          for: indexPath
  //        )
  //        if let promptCell = cell as? SearchSuggestionPromptCell {
  //          promptCell.selectionStyle = .none
  //          promptCell.onOptionSelected = { [weak self] option in
  //            guard let self = self else { return }
  //
  //            self.dataSource.searchEngines?.shouldShowSearchSuggestions = option
  //            self.dataSource.searchEngines?.shouldShowSearchSuggestionsOptIn = false
  //
  //            if option {
  //              self.dataSource.querySuggestClient()
  //            }
  //            self.layoutSuggestionsOptInPrompt()
  //            self.reloadSearchEngines()
  //            self.tableView.reloadData()
  //          }
  //        }
  //      }
  //
  //      guard let tableViewCell = cell else { return UITableViewCell() }
  //      tableViewCell.separatorInset = .zero
  //
  //      return tableViewCell
  //    case .searchSuggestions:
  //      var cell: UITableViewCell?
  //
  //      if isBraveSearchPrompt(for: indexPath) {
  //        cell = createSearchSuggestionPromotionCell()
  //      } else {
  //        cell = tableView.dequeueReusableCell(
  //          withIdentifier: SuggestionCell.identifier,
  //          for: indexPath
  //        )
  //
  //        if let suggestionCell = cell as? SuggestionCell,
  //          let suggestion = dataSource.suggestions[safe: indexPath.row]
  //        {
  //          suggestionCell.setTitle(suggestion)
  //          suggestionCell.separatorInset = UIEdgeInsets(
  //            top: 0.0,
  //            left: view.bounds.width,
  //            bottom: 0.0,
  //            right: -view.bounds.width
  //          )
  //          suggestionCell.openButtonActionHandler = { [weak self] in
  //            guard let self = self else { return }
  //
  //            self.searchDelegate?.searchViewController(self, didLongPressSuggestion: suggestion)
  //          }
  //        }
  //      }
  //
  //      guard let tableViewCell = cell else { return UITableViewCell() }
  //      tableViewCell.separatorInset = .zero
  //
  //      return tableViewCell
  //    }
  //  }
}

// MARK: - UICollectionViewDelegate, UICollectionViewDataSource
extension SearchViewController: UICollectionViewDelegate, UICollectionViewDataSource {
  public func numberOfSections(in collectionView: UICollectionView) -> Int {
    availableSections.count
  }

  public func collectionView(
    _ collectionView: UICollectionView,
    numberOfItemsInSection section: Int
  ) -> Int {
    let section = availableSections[section]

    switch section {
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
        && !dataSource.tabType.isPrivate ? searchSuggestionsCount + 1 : 1  // + 1 for quickBar
    case .openTabsAndHistoryAndBookmarks:
      return dataSource.siteData.count
    case .findInPage:
      return 1
    case .braveSearchPromotion:
      return 1
    }
  }

  public func collectionView(
    _ collectionView: UICollectionView,
    cellForItemAt indexPath: IndexPath
  ) -> UICollectionViewCell {
    let section = availableSections[indexPath.section]
    switch section {
    case .searchSuggestions:
      let cell = collectionView.dequeueReusableCell(for: indexPath) as SearchSuggestionCell
      if indexPath.row == 0 {
        // quick bar
        cell.setTitle(String(format: "Search \"%@\"", dataSource.searchQuery))
      } else {
        if let suggestion = dataSource.suggestions[safe: indexPath.row - 1] {
          cell.setTitle(suggestion)
          cell.openButtonActionHandler = { [weak self] in
            guard let self = self else { return }

            self.searchDelegate?.searchViewController(self, didLongPressSuggestion: suggestion)
          }
        }
      }

      return cell
    case .findInPage:
      let cell = collectionView.dequeueReusableCell(for: indexPath) as SearchFindInPageCell
      cell.setTitle(dataSource.searchQuery)
      return cell
    case .openTabsAndHistoryAndBookmarks:
      let cell = collectionView.dequeueReusableCell(for: indexPath) as SearchOnYourDeviceCell
      let site = dataSource.siteData[indexPath.row]
      cell.setSite(site, isPrivateBrowsing: dataSource.tabType.isPrivate)

      return cell
    default:
      let cell = collectionView.dequeueReusableCell(for: indexPath) as SearchSuggestionCell
      cell.setTitle("a placeholder")

      return cell
    }
  }

  public func collectionView(
    _ collectionView: UICollectionView,
    viewForSupplementaryElementOfKind kind: String,
    at indexPath: IndexPath
  ) -> UICollectionReusableView {
    let section = availableSections[indexPath.section]

    if kind == UICollectionView.elementKindSectionHeader {
      if let header =
        collectionView
        .dequeueReusableSupplementaryView(
          ofKind: kind,
          withReuseIdentifier: "search_header",
          for: indexPath
        ) as? SearchSectionHeaderView
      {
        switch section {
        case .searchSuggestions, .braveSearchPromotion:
          assertionFailure("no header for search suggestion")
        case .searchSuggestionsOptIn:
          header.setTitle("Search Suggestion")
        case .findInPage:
          header.setTitle("On This Page")
        case .openTabsAndHistoryAndBookmarks:
          header.setTitle("On Your Device")
        }
        return header
      }
    } else if kind == UICollectionView.elementKindSectionFooter {
      let footer =
        collectionView
        .dequeueReusableSupplementaryView(
          ofKind: kind,
          withReuseIdentifier: "in_your_device_footer",
          for: indexPath
        )
      let tapGesture = UITapGestureRecognizer(
        target: self,
        action: #selector(onShowMorePressed)
      )
      footer.addGestureRecognizer(tapGesture)
      footer.isUserInteractionEnabled = true

      return footer
    }
    return
      collectionView
      .dequeueReusableSupplementaryView(
        ofKind: kind,
        withReuseIdentifier: "search_header",
        for: indexPath
      )
  }

  public func collectionView(
    _ collectionView: UICollectionView,
    didSelectItemAt indexPath: IndexPath
  ) {
    guard let section = availableSections[safe: indexPath.section] else { return }

    switch section {
    case .searchSuggestionsOptIn: return
    case .searchSuggestions:
      if indexPath.row == 0 {
        // quick bar
        submitSeachTemplateQuery(isBraveSearchPromotion: false)
      } else if !isBraveSearchPrompt(for: indexPath) {
        // Assume that only the default search engine can provide search suggestions.
        let engine = dataSource.searchEngines?.defaultEngine(
          forType: dataSource.isPrivate ? .privateMode : .standard
        )
        let suggestion = dataSource.suggestions[indexPath.row - 1]

        var url = URIFixup.getURL(suggestion)
        if url == nil {
          url = engine?.searchURLForQuery(suggestion)
        }

        if let url = url {
          if !dataSource.isPrivate {
            RecentSearch.addItem(type: .website, text: suggestion, websiteUrl: url.absoluteString)
          }
          searchDelegate?.searchViewController(self, didSelectURL: url)
        }
      }
    case .findInPage:
      let localSearchQuery = dataSource.searchQuery.lowercased()
      searchDelegate?.searchViewController(self, shouldFindInPage: localSearchQuery)
    case .openTabsAndHistoryAndBookmarks:
      let site = dataSource.siteData[indexPath.row]
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
    case .braveSearchPromotion:
      return
    }
  }
}

// MARK: - SearchSuggestionDataSourceDelegate

extension SearchViewController: SearchSuggestionDataSourceDelegate {
  func searchSuggestionDataSourceReloaded() {
    collectionView.reloadData()
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
  @objc func onSuggestionLongPressed(_ gestureRecognizer: UILongPressGestureRecognizer) {
    if gestureRecognizer.state == .began {
      let location = gestureRecognizer.location(in: self.collectionView)
      if let indexPath = collectionView.indexPathForItem(at: location),
        let section = availableSections[safe: indexPath.section],
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
