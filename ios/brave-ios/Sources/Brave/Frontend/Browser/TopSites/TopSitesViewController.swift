// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Combine
import CoreData
import Data
import Observation
import Preferences
import Shared
import UIKit
import os.log

class TopSitesCompositionalLayout: UICollectionViewCompositionalLayout {
  let browserColors: BrowserColors

  init(
    browserColors: BrowserColors,
    sectionProvider: @escaping UICollectionViewCompositionalLayoutSectionProvider,
    configuration: UICollectionViewCompositionalLayoutConfiguration
  ) {
    self.browserColors = browserColors
    super.init(sectionProvider: sectionProvider, configuration: configuration)
    self.register(TopSitesSectionBackgroundView.self, forDecorationViewOfKind: "background")
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func layoutAttributesForElements(
    in rect: CGRect
  ) -> [UICollectionViewLayoutAttributes]? {
    guard let attributes = super.layoutAttributesForElements(in: rect) else { return nil }

    var modifiedAttributes: [UICollectionViewLayoutAttributes] = []

    for attr in attributes {
      if attr.representedElementKind == "background",
        let indexPath = attr.indexPath as IndexPath?
      {
        let customAttr = TopSitesSectionBackgroundLayoutAttribute(
          forDecorationViewOfKind: "background",
          with: indexPath
        )
        customAttr.frame = attr.frame
        customAttr.zIndex = attr.zIndex
        customAttr.backgroundColour =
          browserColors.favoritesAndSearchScreenSectionBackground
        customAttr.groupBackgroundColour =
          browserColors.favoritesAndSearchScreenSectionGroupBackground
        modifiedAttributes.append(customAttr)
      } else {
        modifiedAttributes.append(attr)
      }
    }

    return modifiedAttributes
  }
}

class TopSitesViewController: UIViewController {

  // UI Properties
  private let layoutConfig = UICollectionViewCompositionalLayoutConfiguration().then {
    $0.interSectionSpacing = 8.0
  }
  private lazy var compositionLayout = TopSitesCompositionalLayout(
    browserColors: privateBrowsingManager.browserColors,
    sectionProvider: { [weak self] sectionIndex, environment in
      guard let self else { return nil }
      let traitCollection = environment.traitCollection
      let section = self.availableSections[sectionIndex]
      switch section {
      case .topSites:
        return self.favoritesLayoutSection(
          contentSizeCategory: traitCollection.preferredContentSizeCategory
        )
      case .recentSearches:
        return self.recentSearchesLayoutSection()
      case .recentSearchesOptIn:
        return self.recentSearchesOptInLayoutSection()
      }
    },
    configuration: layoutConfig
  )
  lazy var collectionView: UICollectionView = UICollectionView(
    frame: .zero,
    collectionViewLayout: compositionLayout
  )
  private let backgroundView = UIView()

  // Data Source
  private typealias DataSource = UICollectionViewDiffableDataSource<
    TopSitesSection, TopSitesDataWrapper
  >
  private typealias Snapshot = NSDiffableDataSourceSnapshot<TopSitesSection, TopSitesDataWrapper>
  private lazy var dataSource: DataSource = DataSource(
    collectionView: collectionView,
    cellProvider: { [weak self] collectionView, indexPath, wrapper -> UICollectionViewCell? in
      self?.cellProvider(
        collectionView: collectionView,
        indexPath: indexPath,
        wrapper: wrapper
      )
    }
  )

  // Actions
  var topSiteAction: (TopSiteAction) -> Void
  var recentSearchAction: (RecentSearch?, Bool) -> Void

  // Fetched Result
  private let recentSearchesFRC = RecentSearch.frc().then {
    $0.fetchRequest.fetchLimit = 5
  }

  private var preferenceBeingObserved = false

  var availableSections: [TopSitesSection] {
    var sections = [TopSitesSection]()

    if !tiles.isEmpty {
      sections.append(.topSites)
    }

    if !privateBrowsingManager.isPrivateBrowsing
      && Preferences.Search.shouldShowRecentSearches.value,
      RecentSearch.totalCount() > 0
    {
      sections.append(.recentSearches)
    } else if !privateBrowsingManager.isPrivateBrowsing
      && Preferences.Search.shouldShowRecentSearchesOptIn.value
    {
      sections.append(.recentSearchesOptIn)
    }
    return sections
  }

  // Private Browsing
  var privateBrowsingManager: PrivateBrowsingManager
  private var privateModeCancellable: AnyCancellable?

  // Search Engines
  private let defaultSearchEngine: OpenSearchEngine?

  // Tiles Source
  let tileSource: TopSitesTileSource
  /// The tiles the collection view is currently showing. Kept separate from the source so that the
  /// snapshot and the items it describes can't disagree while a reload is pending.
  private(set) var tiles: [TopSiteTile] = []

  init(
    privateBrowsingManager: PrivateBrowsingManager,
    defaultSearchEngine: OpenSearchEngine?,
    tileSource: TopSitesTileSource,
    topSiteAction: @escaping (TopSiteAction) -> Void,
    recentSearchAction: @escaping (RecentSearch?, Bool) -> Void
  ) {
    self.topSiteAction = topSiteAction
    self.recentSearchAction = recentSearchAction
    self.privateBrowsingManager = privateBrowsingManager
    self.defaultSearchEngine = defaultSearchEngine
    self.tileSource = tileSource

    super.init(nibName: nil, bundle: nil)

    recentSearchesFRC.delegate = self

    KeyboardHelper.defaultHelper.addDelegate(self)

    Preferences.Search.shouldShowRecentSearches.observe(from: self)
    Preferences.Search.shouldShowRecentSearchesOptIn.observe(from: self)

    privateModeCancellable = privateBrowsingManager
      .$isPrivateBrowsing
      .removeDuplicates()
      .receive(on: RunLoop.main)
      .sink(receiveValue: { [weak self] _ in
        guard let self = self else { return }
        self.updateColors()
      })

    collectionView.do {
      $0.register(TopSitesCollectionViewCell.self)
      $0.register(TopSitesRecentSearchCell.self)
      $0.register(SearchActionsCell.self)
      $0.register(
        TopSitesHeaderView.self,
        forSupplementaryViewOfKind: UICollectionView.elementKindSectionHeader,
        withReuseIdentifier: "fav_header"
      )
      $0.register(
        TopSitesRecentSearchHeaderView.self,
        forSupplementaryViewOfKind: UICollectionView.elementKindSectionHeader,
        withReuseIdentifier: "recent_searches_header"
      )
      $0.register(
        TopSitesRecentSearchFooterView.self,
        forSupplementaryViewOfKind: UICollectionView.elementKindSectionFooter,
        withReuseIdentifier: "recent_search_footer"
      )
      $0.contentInset = .init(top: 8, left: 0, bottom: 8, right: 0)
    }

    dataSource.supplementaryViewProvider = { [weak self] collectionView, kind, indexPath in
      self?.supplementaryViewProvider(
        collectionView: collectionView,
        kind: kind,
        indexPath: indexPath
      )
    }

    updateTiles()
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    doLayout()
    setTheme()

    updateUIWithSnapshot()
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()

    if let state = KeyboardHelper.defaultHelper.currentState {
      updateKeyboardInset(state, animated: false)
    }
  }

  private func favoritesLayoutSection(
    contentSizeCategory: UIContentSizeCategory
  ) -> NSCollectionLayoutSection {
    let itemSize: NSCollectionLayoutSize
    let groupSize: NSCollectionLayoutSize
    if contentSizeCategory <= .large {
      itemSize = NSCollectionLayoutSize(
        widthDimension: .absolute(64),
        heightDimension: .estimated(95)
      )
      groupSize = NSCollectionLayoutSize(
        widthDimension: .fractionalWidth(1),
        heightDimension: .estimated(114)
      )
    } else if contentSizeCategory > .large
      && contentSizeCategory < .extraExtraExtraLarge
    {
      itemSize = NSCollectionLayoutSize(
        widthDimension: .absolute(80),
        heightDimension: .estimated(115)
      )
      groupSize = NSCollectionLayoutSize(
        widthDimension: .fractionalWidth(1),
        heightDimension: .estimated(150)
      )
    } else {
      itemSize = NSCollectionLayoutSize(
        widthDimension: .absolute(96),
        heightDimension: .estimated(135)
      )
      groupSize = NSCollectionLayoutSize(
        widthDimension: .fractionalWidth(1),
        heightDimension: .estimated(187)
      )
    }
    let item = NSCollectionLayoutItem(layoutSize: itemSize)
    let group = NSCollectionLayoutGroup.horizontal(layoutSize: groupSize, subitems: [item])
    group.interItemSpacing = .flexible(8)
    group.contentInsets = .init(top: 0, leading: 16, bottom: 0, trailing: 16)
    let section = NSCollectionLayoutSection(group: group)
    section.contentInsets = .init(top: 8, leading: 8, bottom: 8, trailing: 8)

    let headerItemSize = NSCollectionLayoutSize(
      widthDimension: .fractionalWidth(1),
      heightDimension: .absolute(46)
    )
    let headerItem = NSCollectionLayoutBoundarySupplementaryItem(
      layoutSize: headerItemSize,
      elementKind: UICollectionView.elementKindSectionHeader,
      alignment: .top
    )
    headerItem.contentInsets = .init(top: 0, leading: 20, bottom: 0, trailing: 20)
    section.boundarySupplementaryItems = [headerItem]

    let backgroundItem = NSCollectionLayoutDecorationItem.background(elementKind: "background")
    backgroundItem.contentInsets = .init(top: 0, leading: 8, bottom: 0, trailing: 8)
    section.decorationItems = [backgroundItem]

    return section
  }

  private func recentSearchesLayoutSection() -> NSCollectionLayoutSection {
    let itemSize = NSCollectionLayoutSize(
      widthDimension: .fractionalWidth(1),
      heightDimension: .estimated(44)
    )
    let item = NSCollectionLayoutItem(layoutSize: itemSize)
    let groupSize = NSCollectionLayoutSize(
      widthDimension: .fractionalWidth(1),
      heightDimension: .estimated(44)
    )
    let group = NSCollectionLayoutGroup.vertical(layoutSize: groupSize, subitems: [item])
    let section = NSCollectionLayoutSection(group: group)
    section.contentInsets = .init(top: 8, leading: 8, bottom: 8, trailing: 8)

    var supplementaryItems = [NSCollectionLayoutBoundarySupplementaryItem]()

    let headerItemSize = NSCollectionLayoutSize(
      widthDimension: .fractionalWidth(1),
      heightDimension: .absolute(46)
    )
    let headerItem = NSCollectionLayoutBoundarySupplementaryItem(
      layoutSize: headerItemSize,
      elementKind: UICollectionView.elementKindSectionHeader,
      alignment: .top
    )
    headerItem.contentInsets = .init(top: 0, leading: 20, bottom: 0, trailing: 20)
    supplementaryItems.append(headerItem)

    if let fetchedObjects = recentSearchesFRC.fetchedObjects {
      let totalCount = RecentSearch.totalCount()
      if fetchedObjects.count < totalCount {
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

    let backgroundItem = NSCollectionLayoutDecorationItem.background(elementKind: "background")
    backgroundItem.contentInsets = .init(top: 0, leading: 8, bottom: 0, trailing: 8)
    section.decorationItems = [backgroundItem]

    return section
  }

  private func recentSearchesOptInLayoutSection() -> NSCollectionLayoutSection {
    let itemSize = NSCollectionLayoutSize(
      widthDimension: .fractionalWidth(1),
      heightDimension: .estimated(184)
    )
    let item = NSCollectionLayoutItem(layoutSize: itemSize)
    let groupSize = NSCollectionLayoutSize(
      widthDimension: .fractionalWidth(1),
      heightDimension: .estimated(184)
    )
    let group = NSCollectionLayoutGroup.horizontal(layoutSize: groupSize, subitems: [item])
    let section = NSCollectionLayoutSection(group: group)
    section.contentInsets = .init(top: 0, leading: 12, bottom: 0, trailing: 12)

    let headerItemSize = NSCollectionLayoutSize(
      widthDimension: .fractionalWidth(1),
      heightDimension: .absolute(46)
    )
    let headerItem = NSCollectionLayoutBoundarySupplementaryItem(
      layoutSize: headerItemSize,
      elementKind: UICollectionView.elementKindSectionHeader,
      alignment: .top
    )
    headerItem.contentInsets = .init(top: 0, leading: 16, bottom: 0, trailing: 16)
    section.boundarySupplementaryItems = [headerItem]

    let backgroundItem = NSCollectionLayoutDecorationItem.background(elementKind: "background")
    backgroundItem.contentInsets = .init(top: 0, leading: 8, bottom: 0, trailing: 8)
    section.decorationItems = [backgroundItem]

    return section
  }

  private func doLayout() {
    view.addSubview(backgroundView)
    view.addSubview(collectionView)

    backgroundView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    collectionView.snp.makeConstraints {
      $0.top.leading.trailing.equalToSuperview()
      $0.bottom.equalTo(view.safeArea.bottom)
    }
  }

  private func setTheme() {
    collectionView.do {
      $0.alwaysBounceVertical = true
      $0.backgroundColor = .clear
      $0.dataSource = dataSource
      $0.delegate = self
      $0.dragDelegate = self
      $0.dropDelegate = self

      // Drag should be enabled to rearrange favourite
      $0.dragInteractionEnabled = true
      $0.keyboardDismissMode = .interactive
      $0.alwaysBounceVertical = true
      $0.showsHorizontalScrollIndicator = false
    }

    updateColors()
  }

  private func updateColors() {
    let browserColors = privateBrowsingManager.browserColors
    backgroundView.backgroundColor = browserColors.favoritesAndSearchScreenBackground
  }

  private func fetchRecentSearches() {
    if Preferences.Search.shouldShowRecentSearches.value {
      do {
        try recentSearchesFRC.performFetch()
      } catch {
        Logger.module.error("Recent Searches fetch error: \(error.localizedDescription))")
      }
    }
  }
}

// MARK: - UICollectionViewDataSource & UICollectionViewDelegateFlowLayout

extension TopSitesViewController: UICollectionViewDelegateFlowLayout {

  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    guard let section = availableSections[safe: indexPath.section] else {
      assertionFailure("Invalid Section")
      return
    }

    switch section {
    case .recentSearchesOptIn:
      break
    case .topSites:
      guard let tile = tiles[safe: indexPath.item] else {
        return
      }
      topSiteAction(.opened(url: tile.url, isFavorite: tile.isFavorite))
    case .recentSearches:
      guard let searchItem = recentSearchesFRC.fetchedObjects?[safe: indexPath.item] else {
        return
      }
      recentSearchAction(searchItem, true)
    }
  }

  func collectionView(
    _ collectionView: UICollectionView,
    targetIndexPathForMoveFromItemAt currentIndexPath: IndexPath,
    toProposedIndexPath proposedIndexPath: IndexPath
  ) -> IndexPath {
    currentIndexPath.section == proposedIndexPath.section ? proposedIndexPath : currentIndexPath
  }
}

// MARK: - Action

extension TopSitesViewController {
  func onOpenRecentSearch(_ recentSearch: RecentSearch) {
    recentSearchAction(recentSearch, false)
  }

  @objc
  func onRecentSearchShowPressed() {
    // User enabled recent searches
    Preferences.Search.shouldShowRecentSearches.value = true
    Preferences.Search.shouldShowRecentSearchesOptIn.value = false
  }

  @objc
  func onRecentSearchHidePressed(_ button: UIButton) {
    // User doesn't want to see the recent searches option again
    Preferences.Search.shouldShowRecentSearchesOptIn.value = false
  }

  @objc
  func onRecentSearchShowMorePressed() {
    // User already had recent searches enabled, and they want to see more results
    NSFetchedResultsController<RecentSearch>.deleteCache(withName: recentSearchesFRC.cacheName)
    recentSearchesFRC.fetchRequest.fetchLimit = 0
    fetchRecentSearches()
  }

  @objc
  func onRecentSearchClearPressed(_ button: UIButton) {
    // User cleared recent searches
    let alert = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
    alert.addAction(
      UIAlertAction(
        title: Strings.recentSearchClearAlertButton,
        style: .default,
        handler: { _ in
          RecentSearch.removeAll()
        }
      )
    )

    alert.popoverPresentationController?.sourceView = button
    alert.popoverPresentationController?.permittedArrowDirections = [.down, .up]

    alert.addAction(
      UIAlertAction(title: Strings.cancelButtonTitle, style: .destructive, handler: nil)
    )
    present(alert, animated: true, completion: nil)
  }
}

// MARK: - Preference Observer

extension TopSitesViewController: PreferencesObserver {
  func preferencesDidChange(for key: String) {
    preferenceBeingObserved = true
    updateUIWithSnapshot()
  }
}

// MARK: -  NSFetchedResultsControllerDelegate + Diffable DataSource

extension TopSitesViewController: NSFetchedResultsControllerDelegate {
  private var favoritesSectionExists: Bool {
    availableSections.contains(.topSites)
  }

  private var recentSearchesSectionExists: Bool {
    availableSections.contains(.recentSearches)
  }

  private var recentSearchesOptInSectionExists: Bool {
    availableSections.contains(.recentSearchesOptIn)
  }

  func updateUIWithSnapshot(animated: Bool = false) {
    fetchRecentSearches()

    var snapshot = Snapshot()
    snapshot.appendSections(availableSections)

    if favoritesSectionExists {
      snapshot.appendItems(
        tiles.map { .topSites(TopSitesTileDiffable($0)) },
        toSection: .topSites
      )
    }

    if recentSearchesSectionExists, let objects = recentSearchesFRC.fetchedObjects {
      snapshot.appendItems(
        objects.compactMap({ .recentSearch($0.objectID) }),
        toSection: .recentSearches
      )
    } else if recentSearchesOptInSectionExists {
      snapshot.appendItems([.recentSearchOptIn], toSection: .recentSearchesOptIn)
    }

    dataSource.apply(snapshot, animatingDifferences: animated)
  }

  func controller(
    _ controller: NSFetchedResultsController<NSFetchRequestResult>,
    didChangeContentWith snapshot: NSDiffableDataSourceSnapshotReference
  ) {
    // Do not update section contents preference value is changed
    guard !preferenceBeingObserved else {
      preferenceBeingObserved = false
      return
    }

    let currentSnapshot = dataSource.snapshot()

    // Skip applying snapshots before first `initialSnapshotSetup` is made.
    if currentSnapshot.numberOfSections == 0 {
      return
    }

    var newSnapshot = Snapshot()
    newSnapshot.appendSections(availableSections)

    guard let ids = snapshot.itemIdentifiers as? [NSManagedObjectID] else { return }

    if controller === recentSearchesFRC {
      if favoritesSectionExists {
        // New snapshot is created, items from the other frc must be added to it.
        newSnapshot.appendItems(
          currentSnapshot.itemIdentifiers(inSection: .topSites),
          toSection: .topSites
        )
      }

      if recentSearchesSectionExists {
        var items = [TopSitesDataWrapper]()

        ids.forEach {
          if RecentSearch.get(with: $0) != nil {
            items.append(.recentSearch($0))
          }
        }

        newSnapshot.appendItems(items, toSection: .recentSearches)

        // Update recent searches header view.
        // This must be called if previous snapshot had recent searches section already added.
        // Unfortunate side effect is that this will reload all items within the section too.
        if dataSource.snapshot().indexOfSection(.recentSearches) != nil {
          newSnapshot.reloadSections([.recentSearches])
        }
      }
    }

    dataSource.apply(newSnapshot)
  }

  private func cellProvider(
    collectionView: UICollectionView,
    indexPath: IndexPath,
    wrapper: TopSitesDataWrapper
  ) -> UICollectionViewCell? {

    switch wrapper {
    case .topSites(let topsitesWrapper):
      guard let tile = tiles.first(where: { $0.id == topsitesWrapper.id }) else { return nil }

      let cell = collectionView.dequeueReusableCell(for: indexPath) as TopSitesCollectionViewCell

      cell.isPrivateBrowsing = privateBrowsingManager.isPrivateBrowsing
      cell.textLabel.text = tile.title ?? tile.url.absoluteString
      cell.imageView.loadFavicon(
        siteURL: tile.url,
        isPrivateBrowsing: self.privateBrowsingManager.isPrivateBrowsing
      )
      cell.accessibilityLabel = cell.textLabel.text

      return cell
    case .recentSearch(let objectID):
      guard let recentSearch = RecentSearch.get(with: objectID) else { return nil }

      let cell = collectionView.dequeueReusableCell(for: indexPath) as TopSitesRecentSearchCell

      guard let searchType = RecentSearchType(rawValue: recentSearch.searchType) else {
        cell.setTitle(recentSearch.text)
        return cell
      }

      cell.openButtonAction = { [weak self] in
        self?.onOpenRecentSearch(recentSearch)
      }

      switch searchType {
      case .text:
        cell.setTitle(recentSearch.text)
      case .qrCode:
        if let text = recentSearch.text ?? recentSearch.websiteUrl {
          let title = NSMutableAttributedString(
            string: "\(Strings.recentSearchScanned) ",
            attributes: [.font: UIFont.systemFont(ofSize: 15.0, weight: .semibold)]
          )
          title.append(
            NSAttributedString(
              string: "\"\(text)\"",
              attributes: [.font: UIFont.systemFont(ofSize: 15.0)]
            )
          )
          cell.setAttributedTitle(title)
        }
      case .website:
        if let text = recentSearch.text,
          let websiteUrl = recentSearch.websiteUrl
        {
          let website =
            URL(string: websiteUrl)?.baseDomain ?? URL(string: websiteUrl)?.host ?? websiteUrl
          var searchIsUsingDefaultEngine = false
          if let dse = defaultSearchEngine,
            dse.searchTemplate.contains(website)
          {
            searchIsUsingDefaultEngine = true
          }

          let title = NSMutableAttributedString(
            string: text,
            attributes: [.font: UIFont.systemFont(ofSize: 15.0)]
          )
          if !searchIsUsingDefaultEngine {
            title.append(
              NSAttributedString(
                string: " \(Strings.recentSearchQuickSearchOnWebsite) ",
                attributes: [.font: UIFont.systemFont(ofSize: 15.0, weight: .semibold)]
              )
            )
            title.append(
              NSAttributedString(
                string: website,
                attributes: [.font: UIFont.systemFont(ofSize: 15.0)]
              )
            )
          }
          cell.setAttributedTitle(title)
        } else if let websiteUrl = recentSearch.websiteUrl {
          cell.setTitle(websiteUrl)
        } else {
          cell.setTitle(recentSearch.text)
        }
      }

      return cell
    case .recentSearchOptIn:
      let cell =
        collectionView.dequeueReusableCell(for: indexPath) as SearchActionsCell
      cell.do {
        $0.titleLabel.text = Strings.recentSearchSectionTitle
        $0.subtitleLabel.text = Strings.recentSearchSectionDescription
        $0.imageView.image = UIImage(named: "recent-search-opt-in", in: .module, with: nil)
        $0.primaryButton.setTitle(
          Strings.recentSearchShow,
          for: .normal
        )
        $0.primaryButton.addTarget(
          self,
          action: #selector(onRecentSearchShowPressed),
          for: .touchUpInside
        )
        $0.secondaryButton.setTitle(
          Strings.recentSearchHide,
          for: .normal
        )
        $0.secondaryButton.addTarget(
          self,
          action: #selector(onRecentSearchHidePressed(_:)),
          for: .touchUpInside
        )
      }

      return cell
    }
  }

  private func supplementaryViewProvider(
    collectionView: UICollectionView,
    kind: String,
    indexPath: IndexPath
  ) -> UICollectionReusableView? {
    guard let section = availableSections[safe: indexPath.section] else {
      assertionFailure("Invalid Section")
      return UICollectionReusableView()
    }

    if kind == UICollectionView.elementKindSectionHeader {
      switch section {
      case .topSites:
        if let header = collectionView.dequeueReusableSupplementaryView(
          ofKind: kind,
          withReuseIdentifier: "fav_header",
          for: indexPath
        ) as? TopSitesHeaderView {
          header.isPrivateBrowsing = privateBrowsingManager.isPrivateBrowsing
          return header
        }
      case .recentSearches, .recentSearchesOptIn:
        if let header = collectionView.dequeueReusableSupplementaryView(
          ofKind: kind,
          withReuseIdentifier: "recent_searches_header",
          for: indexPath
        ) as? TopSitesRecentSearchHeaderView {
          header.clearButton.addTarget(
            self,
            action: #selector(onRecentSearchClearPressed(_:)),
            for: .touchUpInside
          )

          header.setButtonVisibility(
            showClearButtonVisible: section != .recentSearchesOptIn && RecentSearch.totalCount() > 0
          )

          return header
        }
      }
    } else if kind == UICollectionView.elementKindSectionFooter {
      let footer =
        collectionView
        .dequeueReusableSupplementaryView(
          ofKind: kind,
          withReuseIdentifier: "recent_search_footer",
          for: indexPath
        )
      let tapGesture = UITapGestureRecognizer(
        target: self,
        action: #selector(onRecentSearchShowMorePressed)
      )
      footer.addGestureRecognizer(tapGesture)
      footer.isUserInteractionEnabled = true

      return footer
    }
    return UICollectionReusableView()
  }
}

extension TopSitesViewController {
  private func updateTiles() {
    tiles = withObservationTracking {
      tileSource.tiles
    } onChange: { [weak self] in
      DispatchQueue.main.async {
        self?.updateTiles()
        self?.updateUIWithSnapshot(animated: true)
      }
    }
  }
}
