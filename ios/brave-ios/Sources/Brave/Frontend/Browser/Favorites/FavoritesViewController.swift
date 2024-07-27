// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Combine
import CoreData
import Data
import Preferences
import Shared
import UIKit
import os.log

class FavoritesViewController: UIViewController {

  // UI Properties
  let collectionView: UICollectionView
  private let backgroundView = UIVisualEffectView(
    effect: UIBlurEffect(style: .systemUltraThinMaterial)
  )

  private let layout = UICollectionViewFlowLayout().then {
    $0.sectionInset = UIEdgeInsets(top: 12, left: 0, bottom: 22, right: 0)
    $0.minimumInteritemSpacing = 0
    $0.minimumLineSpacing = 8
  }

  private var favoriteGridSize: CGSize = .zero

  // Data Source
  private typealias DataSource = UICollectionViewDiffableDataSource<
    FavoritesSection, FavoritesDataWrapper
  >
  private typealias Snapshot = NSDiffableDataSourceSnapshot<FavoritesSection, FavoritesDataWrapper>
  private lazy var dataSource =
    DataSource(
      collectionView: self.collectionView,
      cellProvider: { [weak self] collectionView, indexPath, wrapper -> UICollectionViewCell? in
        self?.cellProvider(collectionView: collectionView, indexPath: indexPath, wrapper: wrapper)
      }
    )

  // Actions
  var bookmarkAction: (Favorite, BookmarksAction) -> Void
  var recentSearchAction: (RecentSearch?, Bool) -> Void

  // Fetched Result
  let favoritesFRC = Favorite.frc()
  private let recentSearchesFRC = RecentSearch.frc().then {
    $0.fetchRequest.fetchLimit = 5
  }

  private var preferenceBeingObserved = false

  var availableSections: [FavoritesSection] {
    var sections = [FavoritesSection]()
    if UIPasteboard.general.hasStrings || UIPasteboard.general.hasURLs {
      sections.append(.pasteboard)
    }

    if let favoritesObjects = favoritesFRC.fetchedObjects, !favoritesObjects.isEmpty {
      sections.append(.favorites)
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

  init(
    privateBrowsingManager: PrivateBrowsingManager,
    bookmarkAction: @escaping (Favorite, BookmarksAction) -> Void,
    recentSearchAction: @escaping (RecentSearch?, Bool) -> Void
  ) {
    self.bookmarkAction = bookmarkAction
    self.recentSearchAction = recentSearchAction
    self.privateBrowsingManager = privateBrowsingManager
    collectionView = UICollectionView(frame: .zero, collectionViewLayout: layout)

    super.init(nibName: nil, bundle: nil)

    collectionView.do {
      $0.register(
        FavoritesRecentSearchClipboardHeaderView.self,
        forSupplementaryViewOfKind: UICollectionView.elementKindSectionHeader,
        withReuseIdentifier: "pasteboard_header"
      )
      $0.register(FavoritesCell.self)
      $0.register(FavoritesRecentSearchCell.self)
      $0.register(
        FavoritesHeaderView.self,
        forSupplementaryViewOfKind: UICollectionView.elementKindSectionHeader,
        withReuseIdentifier: "fav_header"
      )
      $0.register(
        FavoritesRecentSearchHeaderView.self,
        forSupplementaryViewOfKind: UICollectionView.elementKindSectionHeader,
        withReuseIdentifier: "recent_searches_header"
      )

      $0.register(
        FavoritesRecentSearchOptInHeaderView.self,
        forSupplementaryViewOfKind: UICollectionView.elementKindSectionHeader,
        withReuseIdentifier: "recent_searches_opt-in_header"
      )

    }

    favoritesFRC.delegate = self
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
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    dataSource.supplementaryViewProvider = { [weak self] collectionView, kind, indexPath in
      self?.supplementaryViewProvider(
        collectionView: collectionView,
        kind: kind,
        indexPath: indexPath
      )
    }

    doLayout()
    setTheme()

    updateUIWithSnapshot()
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()

    calculateAppropriateGrid()

    if let state = KeyboardHelper.defaultHelper.currentState {
      updateKeyboardInset(state, animated: false)
    }
  }

  override func viewWillLayoutSubviews() {
    super.viewWillLayoutSubviews()

    collectionView.contentInset = collectionView.contentInset.with {
      $0.left = self.view.readableContentGuide.layoutFrame.minX
      $0.right = self.view.readableContentGuide.layoutFrame.minX
    }
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    calculateAppropriateGrid()
  }

  private func doLayout() {
    view.addSubview(backgroundView)
    view.addSubview(collectionView)

    backgroundView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    collectionView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
  }

  private func setTheme() {
    collectionView.do {
      $0.alwaysBounceVertical = true
      $0.contentInset = UIEdgeInsets(top: 24, left: 0, bottom: 0, right: 0)
      $0.backgroundColor = .clear
      $0.dataSource = dataSource
      $0.delegate = self
      $0.dragDelegate = self
      $0.dropDelegate = self

      // Drag should be enabled to rearrange favourite
      $0.dragInteractionEnabled = true
      $0.keyboardDismissMode = .interactive
    }

    updateColors()
  }

  private func updateColors() {
    let browserColors = privateBrowsingManager.browserColors
    // Have to apply a custom alpha here because UIVisualEffectView blurs come with their own tint
    backgroundView.contentView.backgroundColor = browserColors.containerFrostedGlass
      .withAlphaComponent(0.8)
  }

  private func calculateAppropriateGrid() {
    let width =
      collectionView.bounds.width - (layout.sectionInset.left + layout.sectionInset.right)
      - (collectionView.contentInset.left + collectionView.contentInset.right)
    // Want to fit _at least_ 4 on all devices, but on larger devices
    // allowing the cells to be a bit bigger
    let minimumNumberOfColumns = 4
    let minWidth = floor(width / CGFloat(minimumNumberOfColumns))
    // Default width should be 82, but may get smaller or bigger
    var itemSize = CGSize(width: 82, height: FavoritesCell.height(forWidth: 82))
    if minWidth < 82 {
      itemSize = CGSize(
        width: floor(width / 4.0),
        height: FavoritesCell.height(forWidth: floor(width / 4.0))
      )
    } else if traitCollection.horizontalSizeClass == .regular {
      // On iPad's or Max/Plus phones allow the icons to get bigger to an
      // extent
      if width / CGFloat(minimumNumberOfColumns) > 100.0 {
        itemSize = CGSize(width: 100, height: FavoritesCell.height(forWidth: 100))
      }
    }

    favoriteGridSize = itemSize
    layout.invalidateLayout()
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

extension FavoritesViewController: UICollectionViewDelegateFlowLayout {

  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    guard let section = availableSections[safe: indexPath.section] else {
      assertionFailure("Invalid Section")
      return
    }

    switch section {
    case .pasteboard, .recentSearchesOptIn:
      break
    case .favorites:
      guard let bookmark = favoritesFRC.fetchedObjects?[safe: indexPath.item] else {
        return
      }
      bookmarkAction(bookmark, .opened())
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

  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    referenceSizeForHeaderInSection section: Int
  ) -> CGSize {
    guard let section = availableSections[safe: section] else {
      assertionFailure("Invalid Section")
      return .zero
    }

    switch section {
    case .pasteboard:
      return CGSize(width: collectionView.bounds.width, height: 40.0)
    case .favorites:
      return CGSize(width: collectionView.bounds.width, height: 32.0)
    case .recentSearches:
      return CGSize(width: collectionView.bounds.width, height: 22.0)
    case .recentSearchesOptIn:
      return CGSize(width: collectionView.bounds.width, height: 150.0)
    }
  }

  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    sizeForItemAt indexPath: IndexPath
  ) -> CGSize {
    guard let section = availableSections[safe: indexPath.section] else {
      assertionFailure("Invalid Section")
      return .zero
    }

    let collectionViewWidth =
      collectionView.bounds.width - (layout.sectionInset.left + layout.sectionInset.right)
      - (collectionView.contentInset.left + collectionView.contentInset.right)

    switch section {
    case .pasteboard, .recentSearchesOptIn:
      assertionFailure("Pasteboard/Recent Search Opt-In section should have no items")
      return .zero
    case .favorites:
      return favoriteGridSize
    case .recentSearches:
      return CGSize(width: collectionViewWidth, height: 28.0)
    }
  }
}

// MARK: - Action

extension FavoritesViewController {
  func onOpenRecentSearch(_ recentSearch: RecentSearch) {
    recentSearchAction(recentSearch, false)
  }

  @objc
  func onPasteboardAction() {
    recentSearchAction(nil, true)
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

extension FavoritesViewController: PreferencesObserver {
  func preferencesDidChange(for key: String) {
    preferenceBeingObserved = true
    updateUIWithSnapshot()
  }
}

// MARK: -  NSFetchedResultsControllerDelegate + Diffable DataSource

extension FavoritesViewController: NSFetchedResultsControllerDelegate {
  private var favoritesSectionExists: Bool {
    availableSections.contains(.favorites)
  }

  private var recentSearchesSectionExists: Bool {
    availableSections.contains(.recentSearches)
  }

  private func updateUIWithSnapshot() {
    do {
      try favoritesFRC.performFetch()
    } catch {
      Logger.module.error("Favorites fetch error: \(error.localizedDescription))")
    }

    fetchRecentSearches()

    var snapshot = Snapshot()
    snapshot.appendSections(availableSections)

    let fetchedFavorites: [FavoritesDataWrapper]? = favoritesFRC.fetchedObjects?.compactMap {
      return .favorite(
        FavoritesDiffable(objectID: $0.objectID, title: $0.displayTitle, url: $0.url)
      )
    }

    if favoritesSectionExists, let favorites = fetchedFavorites {
      snapshot.appendItems(favorites, toSection: .favorites)
    }

    if recentSearchesSectionExists, let objects = recentSearchesFRC.fetchedObjects {
      snapshot.appendItems(
        objects.compactMap({ .recentSearch($0.objectID) }),
        toSection: .recentSearches
      )
    }

    dataSource.apply(snapshot, animatingDifferences: false)
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

    if controller === favoritesFRC {
      var items = [FavoritesDataWrapper]()

      if favoritesSectionExists {
        ids.forEach {
          // Fetch existing item from the DB then add it to snapshot.
          // This way the snapshot will be able to detect changes on the object.
          // Non existing objects are not added, this simulates removing the item.
          if let existingItem = controller.managedObjectContext.object(with: $0) as? Favorite {
            items.append(
              .favorite(
                FavoritesDiffable(
                  objectID: $0,
                  title: existingItem.title,
                  url: existingItem.url
                )
              )
            )
          }
        }

        newSnapshot.appendItems(items, toSection: .favorites)
      }

      // New snapshot is created, items from the other frc must be added to it.
      if recentSearchesSectionExists {
        newSnapshot.appendItems(
          currentSnapshot.itemIdentifiers(inSection: .recentSearches),
          toSection: .recentSearches
        )
      }
    }

    if controller === recentSearchesFRC {
      if favoritesSectionExists {
        // New snapshot is created, items from the other frc must be added to it.
        newSnapshot.appendItems(
          currentSnapshot.itemIdentifiers(inSection: .favorites),
          toSection: .favorites
        )
      }

      if recentSearchesSectionExists {
        var items = [FavoritesDataWrapper]()

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
    wrapper: FavoritesDataWrapper
  ) -> UICollectionViewCell? {

    switch wrapper {
    case .favorite(let favoriteWrapper):
      guard let favorite = Favorite.get(with: favoriteWrapper.objectID) else { return nil }

      let cell = collectionView.dequeueReusableCell(for: indexPath) as FavoritesCell

      cell.textLabel.text = favorite.displayTitle ?? favorite.url
      if let url = favorite.url?.asURL {
        cell.imageView.loadFavicon(
          siteURL: url,
          isPrivateBrowsing: self.privateBrowsingManager.isPrivateBrowsing
        )
      } else {
        cell.imageView.cancelLoading()
      }
      cell.accessibilityLabel = cell.textLabel.text

      return cell
    case .recentSearch(let objectID):
      guard let recentSearch = RecentSearch.get(with: objectID) else { return nil }

      let cell = collectionView.dequeueReusableCell(for: indexPath) as FavoritesRecentSearchCell

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

          let title = NSMutableAttributedString(
            string: text,
            attributes: [.font: UIFont.systemFont(ofSize: 15.0)]
          )
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
          cell.setAttributedTitle(title)
        } else if let websiteUrl = recentSearch.websiteUrl {
          cell.setTitle(websiteUrl)
        } else {
          cell.setTitle(recentSearch.text)
        }
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
      case .pasteboard:
        if let header = collectionView.dequeueReusableSupplementaryView(
          ofKind: kind,
          withReuseIdentifier: "pasteboard_header",
          for: indexPath
        ) as? FavoritesRecentSearchClipboardHeaderView {
          header.button.removeTarget(self, action: nil, for: .touchUpInside)
          header.button.addTarget(self, action: #selector(onPasteboardAction), for: .touchUpInside)
          return header
        }
      case .favorites:
        return
          collectionView
          .dequeueReusableSupplementaryView(
            ofKind: kind,
            withReuseIdentifier: "fav_header",
            for: indexPath
          )
      case .recentSearches:
        if let header = collectionView.dequeueReusableSupplementaryView(
          ofKind: kind,
          withReuseIdentifier: "recent_searches_header",
          for: indexPath
        ) as? FavoritesRecentSearchHeaderView {
          header.showMoreButton.addTarget(
            self,
            action: #selector(onRecentSearchShowMorePressed),
            for: .touchUpInside
          )

          header.clearButton.addTarget(
            self,
            action: #selector(onRecentSearchClearPressed(_:)),
            for: .touchUpInside
          )

          var showMoreButtonVisible = false

          if let fetchedObjects = recentSearchesFRC.fetchedObjects {
            let totalCount = RecentSearch.totalCount()
            showMoreButtonVisible = fetchedObjects.count < totalCount
          }

          header.setButtonVisibility(
            showMoreButtonVisible: showMoreButtonVisible
          )

          return header
        }
      case .recentSearchesOptIn:
        if let header = collectionView.dequeueReusableSupplementaryView(
          ofKind: kind,
          withReuseIdentifier: "recent_searches_opt-in_header",
          for: indexPath
        ) as? FavoritesRecentSearchOptInHeaderView {

          header.showButton.addTarget(
            self,
            action: #selector(onRecentSearchShowPressed),
            for: .touchUpInside
          )
          header.hideButton.addTarget(
            self,
            action: #selector(onRecentSearchHidePressed(_:)),
            for: .touchUpInside
          )

          return header
        }
      }
    }
    return UICollectionReusableView()
  }
}
