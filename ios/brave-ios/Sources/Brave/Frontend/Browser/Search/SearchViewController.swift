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
import CoreData

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
    didSelectPlaylistItem item: PlaylistItem
  )
  func searchViewController(
    _ searchViewController: SearchViewController,
    didLongPressSuggestion suggestion: String
  )
  func presentQuickSearchEnginesViewController()
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

  // MARK: ReuseIdentifier

  private struct SupplementaryViewReuseIdentifier {
    static let inYourDeviceSectionFooterIdentifer =
      "in_your_device_footer"
    static let searchHeaderIdentifier = "search_header"
  }

  private struct CollectionViewCellReuseIdentifier {
    static let searchOptinSeparatorCellIdentifier = "search_optin_separator"
  }

  // MARK: NSCollectionLayoutDecorationItemElementKind

  private struct CollectionLayoutDecorationItemElementKind {
    static let backgroundWithHeader = "background_with_header"
    static let backgroundPlain = "background_plain"
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
    sectionProvider: { [weak self] sectionIndex, _ in
      guard let self else { return nil }
      let section = self.availableSections[sectionIndex]
      switch section {
      case .searchSuggestionsOptIn:
        return self.searchSuggestionOptinLayoutSection()
      case .searchSuggestions, .findInPage, .openTabsAndHistoryAndBookmarks:
        return self.searchLayoutSection(
          headerEnabled: section != .searchSuggestions,
          footerEnabled: section == .openTabsAndHistoryAndBookmarks
        )
      case .braveSearchPromotion:
        return self.braveSearchPromotionLayoutSection()
      }
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
  private let backgroundView = UIView()
  private let searchEngineScrollViewContent = UIView()

  private lazy var suggestionLongPressGesture = UILongPressGestureRecognizer(
    target: self,
    action: #selector(onSuggestionLongPressed(_:))
  )

  weak var searchDelegate: SearchViewControllerDelegate?

  var isUsingBottomBar: Bool = false {
    didSet {
      if !dataSource.isAIChatAvailable && !dataSource.hasQuickSearchEngines {
        layoutCollectionView()
      } else {
        layoutSearchEngineScrollView()
      }
    }
  }

  let dataSource: SearchSuggestionDataSource
  let playlistFRC = PlaylistItem.frc()
  public static var userAgent: String?
  private var browserColors: any BrowserColors
  private var allSiteData = [Site]() {
    didSet {
      updateAvailableOnYourDeviceItems()
    }
  }
  private var showAllSiteData: Bool = false
  private var isSiteDataShowMoreAvailable: Bool {
    availableOnYourDeviceItems.count < allOnYourDeviceItems.count
  }
  private struct OnYourDeviceItem {
    private(set) var site: Site?
    private(set) var playlistItem: PlaylistItem?

    init(site: Site) {
      self.site = site
    }

    init(playlistItem: PlaylistItem) {
      self.playlistItem = playlistItem
    }
  }
  private var allOnYourDeviceItems: [OnYourDeviceItem] = []
  private var availableOnYourDeviceItems: [OnYourDeviceItem] {
    if showAllSiteData {
      return allOnYourDeviceItems
    } else {
      return Array(allOnYourDeviceItems.prefix(5))
    }
  }

  private func updateAvailableOnYourDeviceItems() {
    let playlistItems = playlistFRC.fetchedObjects ?? []
    allOnYourDeviceItems = allSiteData.map { OnYourDeviceItem(site: $0) } + playlistItems.map { OnYourDeviceItem(playlistItem: $0) }
    collectionView.reloadData()
  }

  private var availableSections: [SearchSuggestionDataSource.SearchListSection] {
    var result = [SearchSuggestionDataSource.SearchListSection]()

    if !dataSource.isPrivate
      && dataSource.searchEngines?.shouldShowSearchSuggestionsOptIn == true
    {
      result.append(.searchSuggestionsOptIn)
    } else {
      result.append(.searchSuggestions)
    }

    if dataSource.braveSearchPromotionAvailable {
      result.append(.braveSearchPromotion)
    }

    if let sd = searchDelegate, sd.searchViewControllerAllowFindInPage() {
      result.append(.findInPage)
    }
    if !Preferences.Search.showBrowserSuggestions.value {
    } else if Preferences.Privacy.privateBrowsingOnly.value
      && dataSource.searchEngines?.shouldShowSearchSuggestions == false
    {
    } else if !availableOnYourDeviceItems.isEmpty {
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
    if !dataSource.isAIChatAvailable && !dataSource.hasQuickSearchEngines {
      layoutCollectionView()
    } else {
      layoutSearchEngineScrollView()
    }
  }

  override public func viewDidLoad() {
    super.viewDidLoad()

    view.addSubview(backgroundView)
    view.addSubview(collectionView)

    backgroundView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    backgroundView.backgroundColor = browserColors.browserButtonBackgroundHover
    searchEngineScrollView.backgroundColor = browserColors.chromeBackground

    setupSearchEngineScrollViewIfNeeded()

    collectionView.do {
      $0.register(UICollectionViewCell.self, forCellWithReuseIdentifier: "search_optin_separator")
      $0.register(SearchActionsCell.self)
      $0.register(SearchSuggestionCell.self)
      $0.register(SearchFindInPageCell.self)
      $0.register(SearchOnYourDeviceCell.self)
      $0.register(
        SearchSectionHeaderView.self,
        forSupplementaryViewOfKind: UICollectionView.elementKindSectionHeader,
        withReuseIdentifier: SupplementaryViewReuseIdentifier.searchHeaderIdentifier
      )
      $0.register(
        FavoritesRecentSearchFooterView.self,
        forSupplementaryViewOfKind: UICollectionView.elementKindSectionFooter,
        withReuseIdentifier: SupplementaryViewReuseIdentifier.inYourDeviceSectionFooterIdentifer
      )
      $0.alwaysBounceVertical = true
      $0.backgroundColor = .clear
      $0.delegate = self
      $0.dataSource = self
      $0.keyboardDismissMode = .interactive
      $0.addGestureRecognizer(suggestionLongPressGesture)
      $0.contentInset = .init(top: 8, left: 0, bottom: 8, right: 0)
    }

    KeyboardHelper.defaultHelper.addDelegate(self)

    playlistFRC.delegate = self

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
      reloadSearchData()
    }
  }

  // MARK: Internal

  private func setupSearchEngineScrollViewIfNeeded() {
    guard dataSource.isAIChatAvailable || dataSource.hasQuickSearchEngines
    else {
      layoutCollectionView()
      return
    }

    view.addSubview(searchEngineScrollView)
    searchEngineScrollView.addSubview(searchEngineScrollViewContent)

    layoutSearchEngineScrollView()
    layoutCollectionView()

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

  func layoutSearchEngineScrollView() {
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

  private func layoutCollectionView() {
    collectionView.snp.remakeConstraints { make in
      make.top.equalTo(view.snp.top)
      make.leading.trailing.equalTo(view)
      if searchEngineScrollView.superview == nil {
        let keyboardHeight =
          KeyboardHelper.defaultHelper.currentState?.intersectionHeightForView(view) ?? 0
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
      } else {
        make.bottom.equalTo(searchEngineScrollView.snp.top)
      }
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
  }

  func setSearchQuery(query: String, showSearchSuggestions: Bool = true) {
    dataSource.searchQuery = query
    // Do not query suggestions if the text entred is suspicious
    if showSearchSuggestions {
      dataSource.querySuggestClient()
    }
    // fetch media
    if !query.isEmpty {
      playlistFRC.fetchRequest.predicate = NSPredicate(format: "name CONTAINS[c] %@", query)
      try? playlistFRC.performFetch()
      updateAvailableOnYourDeviceItems()
    }
  }

  private func reloadSearchEngines() {
    searchEngineScrollViewContent.subviews.forEach { $0.removeFromSuperview() }
    var leftEdge = searchEngineScrollViewContent.snp.left

    if dataSource.isAIChatAvailable {
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
        make.left.equalTo(leftEdge).offset(
          SearchViewControllerUX.searchButtonMargin * 2
        )
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

      leftEdge = separator.snp.right
    }

    // search engines
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
      make.bottom.equalTo(searchEngineScrollViewContent).inset(
        SearchViewControllerUX.searchButtonMargin
      )
      make.right.equalTo(searchEngineScrollViewContent).inset(
        SearchViewControllerUX.searchButtonMargin * 2
      )
    }
  }

  private func searchSuggestionOptin(enable: Bool) {
    dataSource.searchEngines?.shouldShowSearchSuggestions = enable
    dataSource.searchEngines?.shouldShowSearchSuggestionsOptIn = false

    if enable {
      self.dataSource.querySuggestClient()
    }

    collectionView.reloadData()
  }

  public func loader(dataLoaded data: [Site]) {
    allSiteData = data
    collectionView.reloadData()
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
      heightDimension: .estimated(52)
    )
    let item = NSCollectionLayoutItem(layoutSize: itemSize)
    let groupSize = NSCollectionLayoutSize(
      widthDimension: .fractionalWidth(1),
      heightDimension: .estimated(52)
    )
    let group = NSCollectionLayoutGroup.vertical(layoutSize: groupSize, subitems: [item])
    let section = NSCollectionLayoutSection(group: group)
    section.contentInsets = .init(top: 12, leading: 8, bottom: 12, trailing: 8)

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
      headerItem.contentInsets = .init(top: 0, leading: 20, bottom: 0, trailing: 20)
      supplementaryItems.append(headerItem)

      let backgroundItem = NSCollectionLayoutDecorationItem.background(
        elementKind: CollectionLayoutDecorationItemElementKind.backgroundWithHeader
      )
      backgroundItem.contentInsets = .init(top: 0, leading: 8, bottom: 0, trailing: 8)
      section.decorationItems = [backgroundItem]
    } else {
      let backgroundItem = NSCollectionLayoutDecorationItem.background(
        elementKind: CollectionLayoutDecorationItemElementKind.backgroundPlain
      )
      backgroundItem.contentInsets = .init(top: 0, leading: 8, bottom: 0, trailing: 8)
      section.decorationItems = [backgroundItem]
    }

    if footerEnabled {
      if isSiteDataShowMoreAvailable {
        let footerItemSize = NSCollectionLayoutSize(
          widthDimension: .fractionalWidth(1),
          heightDimension: .estimated(30)
        )
        let footerItem = NSCollectionLayoutBoundarySupplementaryItem(
          layoutSize: footerItemSize,
          elementKind: UICollectionView.elementKindSectionFooter,
          alignment: .bottom
        )
        footerItem.contentInsets = .init(top: 0, leading: 20, bottom: 0, trailing: 20)
        supplementaryItems.append(footerItem)
      }
    }

    section.boundarySupplementaryItems = supplementaryItems

    return section
  }

  private func searchSuggestionOptinLayoutSection() -> NSCollectionLayoutSection {
    let quickBarItemSize = NSCollectionLayoutSize(
      widthDimension: .fractionalWidth(1),
      heightDimension: .estimated(52)
    )
    let quickBarItem = NSCollectionLayoutItem(layoutSize: quickBarItemSize)

    let separatorItemSize = NSCollectionLayoutSize(
      widthDimension: .fractionalWidth(1),
      heightDimension: .absolute(1)
    )
    let separatorItem = NSCollectionLayoutItem(layoutSize: separatorItemSize)
    separatorItem.contentInsets = .init(top: 0, leading: 16, bottom: 0, trailing: 16)

    let optinItemSize = NSCollectionLayoutSize(
      widthDimension: .fractionalWidth(1),
      heightDimension: .estimated(130)
    )
    let optinItem = NSCollectionLayoutItem(layoutSize: optinItemSize)

    let groupSize = NSCollectionLayoutSize(
      widthDimension: .fractionalWidth(1),
      heightDimension: .estimated(183)
    )
    let group = NSCollectionLayoutGroup.vertical(
      layoutSize: groupSize,
      subitems: [quickBarItem, separatorItem, optinItem]
    )
    group.interItemSpacing = .fixed(8)
    let section = NSCollectionLayoutSection(group: group)
    section.contentInsets = .init(top: 16, leading: 12, bottom: 8, trailing: 12)

    let backgroundItem = NSCollectionLayoutDecorationItem.background(
      elementKind: CollectionLayoutDecorationItemElementKind.backgroundPlain
    )
    backgroundItem.contentInsets = .init(top: 0, leading: 8, bottom: 0, trailing: 8)
    section.decorationItems = [backgroundItem]

    return section
  }

  private func braveSearchPromotionLayoutSection() -> NSCollectionLayoutSection {
    let itemSize = NSCollectionLayoutSize(
      widthDimension: .fractionalWidth(1),
      heightDimension: .estimated(1)
    )
    let item = NSCollectionLayoutItem(layoutSize: itemSize)
    let groupSize = NSCollectionLayoutSize(
      widthDimension: .fractionalWidth(1),
      heightDimension: .estimated(168)
    )
    let group = NSCollectionLayoutGroup.vertical(layoutSize: groupSize, subitems: [item])
    let section = NSCollectionLayoutSection(group: group)
    section.contentInsets = .init(top: 4, leading: 12, bottom: 4, trailing: 12)

    let backgroundItem = NSCollectionLayoutDecorationItem.background(
      elementKind: CollectionLayoutDecorationItemElementKind.backgroundPlain
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

    let offset = dataSource.isAIChatAvailable ? 1 : 0  // offset for the Leo button
    let engine = dataSource.quickSearchEngines[index - offset]
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
    self.searchDelegate?.presentQuickSearchEnginesViewController()
  }

  @objc func didClickLeoButton() {
    submitSearchQueryToAIChat()
  }

  @objc func onShowMorePressed() {
    showAllSiteData = true
    collectionView.reloadData()
  }

  @objc func onBraveSearchPromotionOptinButton() {
    submitSeachTemplateQuery(isBraveSearchPromotion: true)
    collectionView.reloadData()
  }

  @objc func onBraveSearchPromtionIgnoreButton() {
    changeBraveSearchPromotionState()
    collectionView.reloadData()
  }

  @objc func onSearchSuggestionEnableButton() {
    searchSuggestionOptin(enable: true)
  }

  @objc func onSearchSuggestionDisableButton() {
    searchSuggestionOptin(enable: false)
  }
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
      return 3  // quick bar + separator + optin
    case .searchSuggestions:
      guard
        let shouldShowSuggestions = dataSource.searchEngines?.shouldShowSearchSuggestions,
        shouldShowSuggestions
      else {
        return 1  // quick bar
      }

      let searchSuggestionsCount = min(
        dataSource.suggestions.count,
        dataSource.maxSearchSuggestions
      )

      return !dataSource.searchQuery.looksLikeAURL()
        && !dataSource.isPrivate ? searchSuggestionsCount + 1 : 1  // + 1 for quickBar
    case .openTabsAndHistoryAndBookmarks:
      return availableOnYourDeviceItems.count
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
    case .searchSuggestionsOptIn:
      if indexPath.row == 0 {
        // quick bar
        let cell = collectionView.dequeueReusableCell(for: indexPath) as SearchSuggestionCell
        cell.setTitle(String(format: Strings.searchQuickBarPrefix, dataSource.searchQuery))

        return cell
      } else if indexPath.row == 1 {
        let cell = collectionView.dequeueReusableCell(
          withReuseIdentifier: CollectionViewCellReuseIdentifier.searchOptinSeparatorCellIdentifier,
          for: indexPath
        )
        cell.backgroundColor = UIColor(braveSystemName: .dividerSubtle)

        return cell
      } else {
        let cell = collectionView.dequeueReusableCell(for: indexPath) as SearchActionsCell
        cell.do {
          $0.layer.cornerRadius = 12
          $0.layer.cornerCurve = .continuous
          $0.titleLabel.text = Strings.recentSearchSuggestionsTitle
          $0.subtitleLabel.text = Strings.searchSuggestionsSubtitle
          $0.imageView.image = UIImage(named: "search-suggestion-opt-in", in: .module, with: nil)
          $0.primaryButton.setTitle(
            Strings.recentSearchEnableSuggestions,
            for: .normal
          )
          $0.primaryButton.addTarget(
            self,
            action: #selector(onSearchSuggestionEnableButton),
            for: .touchUpInside
          )
          $0.secondaryButton.setTitle(
            Strings.recentSearchDisableSuggestions,
            for: .normal
          )
          $0.secondaryButton.addTarget(
            self,
            action: #selector(onSearchSuggestionDisableButton),
            for: .touchUpInside
          )
        }

        return cell
      }
    case .searchSuggestions:
      let cell = collectionView.dequeueReusableCell(for: indexPath) as SearchSuggestionCell
      if indexPath.row == 0 {
        // quick bar
        cell.setTitle(String(format: Strings.searchQuickBarPrefix, dataSource.searchQuery))

        return cell
      } else {  // search suggestion opt-in
        if let suggestion = dataSource.suggestions[safe: indexPath.row - 1] {
          cell.setTitle(suggestion)
          cell.openButtonActionHandler = { [weak self] in
            guard let self = self else { return }

            self.searchDelegate?.searchViewController(self, didLongPressSuggestion: suggestion)
          }
        }

        return cell
      }
    case .braveSearchPromotion:
      let cell = collectionView.dequeueReusableCell(for: indexPath) as SearchActionsCell
      cell.do {
        $0.backgroundColor = UIColor(braveSystemName: .containerInteractive)
        $0.layer.cornerRadius = 12
        $0.titleLabel.text = Strings.BraveSearchPromotion.braveSearchPromotionBannerTitle
        $0.subtitleLabel.text = Strings.BraveSearchPromotion.braveSearchPromotionBannerDescription
        $0.imageView.image = UIImage(named: "brave-search-promotion", in: .module, with: nil)
        $0.primaryButton.setTitle(
          Strings.BraveSearchPromotion.braveSearchPromotionBannerTryButtonTitle,
          for: .normal
        )
        $0.primaryButton.addTarget(
          self,
          action: #selector(onBraveSearchPromotionOptinButton),
          for: .touchUpInside
        )
        $0.secondaryButton.setTitle(
          Preferences.BraveSearch.braveSearchPromotionCompletionState.value
            != BraveSearchPromotionState.maybeLaterUpcomingSession.rawValue
            ? Strings.BraveSearchPromotion.braveSearchPromotionBannerMaybeLaterButtonTitle
            : Strings.BraveSearchPromotion.braveSearchPromotionBannerDismissButtonTitle,
          for: .normal
        )
        $0.secondaryButton.addTarget(
          self,
          action: #selector(onBraveSearchPromtionIgnoreButton),
          for: .touchUpInside
        )
      }

      return cell
    case .findInPage:
      let cell = collectionView.dequeueReusableCell(for: indexPath) as SearchFindInPageCell
      cell.setCellTitle(dataSource.searchQuery)
      return cell
    case .openTabsAndHistoryAndBookmarks:
      let cell = collectionView.dequeueReusableCell(for: indexPath) as SearchOnYourDeviceCell
      let onYourDeviceItem = availableOnYourDeviceItems[indexPath.row]
      if let site = onYourDeviceItem.site {
        cell.setSite(site, isPrivateBrowsing: dataSource.isPrivate)
      } else if let playlistItem = onYourDeviceItem.playlistItem {
        cell.setPlaylistItem(playlistItem)
      }

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
          withReuseIdentifier: SupplementaryViewReuseIdentifier.searchHeaderIdentifier,
          for: indexPath
        ) as? SearchSectionHeaderView
      {
        switch section {
        case .searchSuggestionsOptIn, .searchSuggestions, .braveSearchPromotion:
          assertionFailure("no header for search suggestion")
        case .findInPage:
          header.setTitle(Strings.findOnPageSectionHeader)
        case .openTabsAndHistoryAndBookmarks:
          header.setTitle(Strings.searchHistorySectionHeader)
        }
        return header
      }
    } else if kind == UICollectionView.elementKindSectionFooter {
      let footer =
        collectionView
        .dequeueReusableSupplementaryView(
          ofKind: kind,
          withReuseIdentifier: SupplementaryViewReuseIdentifier.inYourDeviceSectionFooterIdentifer,
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
        withReuseIdentifier: SupplementaryViewReuseIdentifier.searchHeaderIdentifier,
        for: indexPath
      )
  }

  public func collectionView(
    _ collectionView: UICollectionView,
    didSelectItemAt indexPath: IndexPath
  ) {
    guard let section = availableSections[safe: indexPath.section] else { return }

    switch section {
    case .searchSuggestionsOptIn:
      if indexPath.row == 0 {
        // quick bar
        submitSeachTemplateQuery(isBraveSearchPromotion: false)
      }
    case .searchSuggestions:
      if indexPath.row == 0 {
        // quick bar
        submitSeachTemplateQuery(isBraveSearchPromotion: false)
      } else {
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
    case .braveSearchPromotion:
      return
    case .findInPage:
      let localSearchQuery = dataSource.searchQuery.lowercased()
      searchDelegate?.searchViewController(self, shouldFindInPage: localSearchQuery)
    case .openTabsAndHistoryAndBookmarks:
      let onYourDeviceItem = availableOnYourDeviceItems[indexPath.row]
      if let site = onYourDeviceItem.site {
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
      } else if let playlistItem = onYourDeviceItem.playlistItem {
        searchDelegate?.searchViewController(self, didSelectPlaylistItem: playlistItem)
      }
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
        let suggestion = dataSource.suggestions[safe: indexPath.row - 1],
        section == .searchSuggestions
      {
        searchDelegate?.searchViewController(self, didLongPressSuggestion: suggestion)
      }
    }
  }
}

// MARK: - SearchQuickEnginesViewControllerDelegate

extension SearchViewController: SearchQuickEnginesViewControllerDelegate {
  func searchQuickEnginesUpdated() {
    reloadSearchEngines()
  }
}

// MARK: - NSFetchedResultsControllerDelegate

extension SearchViewController: NSFetchedResultsControllerDelegate {
  public func controller(
    _ controller: NSFetchedResultsController<NSFetchRequestResult>,
    didChange anObject: Any,
    at indexPath: IndexPath?,
    for type: NSFetchedResultsChangeType,
    newIndexPath: IndexPath?
  ) {
    updateAvailableOnYourDeviceItems()
  }

  public func controllerDidChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
    updateAvailableOnYourDeviceItems()
  }

  public func controllerWillChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
    updateAvailableOnYourDeviceItems()
  }
}

// MARK: - ButtonScrollView

/// UIScrollView that prevents buttons from interfering with scroll.
private class ButtonScrollView: UIScrollView {
  fileprivate override func touchesShouldCancel(in view: UIView) -> Bool {
    return true
  }
}
