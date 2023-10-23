// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Data
import Shared
import Preferences
import BraveUI
import CoreData
import os.log
import Combine

private class FavoritesHeaderView: UICollectionReusableView {
  let label = UILabel().then {
    $0.text = Strings.recentSearchFavorites
    $0.font = .systemFont(ofSize: 18, weight: .semibold)
  }
  override init(frame: CGRect) {
    super.init(frame: frame)
    addSubview(label)
    label.snp.makeConstraints {
      $0.leading.equalToSuperview().inset(12)
      $0.trailing.lessThanOrEqualToSuperview().inset(12)
      $0.centerY.equalToSuperview()
    }
  }
  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}

class FavoritesViewController: UIViewController {

  /// CoreData's diffable method relies on managed object ID only.
  /// This does not work in our case since the object may be the same but with changed title or order.
  /// This struct stores managed object ID as well as properties that we observer whether they have changed.
  /// Note: `order` property does not have to be stored, favorite's frc handles order updates, it returns items in correct order.
  private struct FavoriteDiffable: Hashable {
    let objectID: NSManagedObjectID
    let title: String?
    let url: String?

    func hash(into hasher: inout Hasher) {
      hasher.combine(objectID)
    }
  }

  /// Favorites VC has two fetch result controllers to pull from.
  /// This enum stores both models.
  private enum DataWrapper: Hashable {
    case favorite(FavoriteDiffable)

    // Recent searches are static, we do not need any wrapper class for them.
    case recentSearch(NSManagedObjectID)
  }

  private typealias DataSource = UICollectionViewDiffableDataSource<Section, DataWrapper>
  private typealias Snapshot = NSDiffableDataSourceSnapshot<Section, DataWrapper>

  private lazy var dataSource =
    DataSource(
      collectionView: self.collectionView,
      cellProvider: { [weak self] collectionView, indexPath, wrapper -> UICollectionViewCell? in

        self?.cellProvider(collectionView: collectionView, indexPath: indexPath, wrapper: wrapper)
      })

  var action: (Favorite, BookmarksAction) -> Void
  var recentSearchAction: (RecentSearch?, Bool) -> Void

  private enum Section: Int, CaseIterable {
    case pasteboard = 0
    case favorites = 1
    case recentSearches = 2
  }

  private let favoritesFRC = Favorite.frc()
  private let recentSearchesFRC = RecentSearch.frc().then {
    $0.fetchRequest.fetchLimit = 5
  }

  private let layout = UICollectionViewFlowLayout().then {
    $0.sectionInset = UIEdgeInsets(top: 12, left: 0, bottom: 22, right: 0)
    $0.minimumInteritemSpacing = 0
    $0.minimumLineSpacing = 8
  }

  private var tabType: TabType
  private var favoriteGridSize: CGSize = .zero
  private let collectionView: UICollectionView
  private let backgroundView = UIVisualEffectView(effect: UIBlurEffect(style: .systemUltraThinMaterial))
  private var hasPasteboardURL = false
  private var privateBrowsingManager: PrivateBrowsingManager
  private var privateModeCancellable: AnyCancellable?

  init(tabType: TabType, privateBrowsingManager: PrivateBrowsingManager, action: @escaping (Favorite, BookmarksAction) -> Void, recentSearchAction: @escaping (RecentSearch?, Bool) -> Void) {
    self.tabType = tabType
    self.action = action
    self.recentSearchAction = recentSearchAction
    self.privateBrowsingManager = privateBrowsingManager
    collectionView = UICollectionView(frame: .zero, collectionViewLayout: layout)

    super.init(nibName: nil, bundle: nil)

    collectionView.register(RecentSearchClipboardHeaderView.self, forSupplementaryViewOfKind: UICollectionView.elementKindSectionHeader, withReuseIdentifier: "pasteboard_header")
    collectionView.register(FavoriteCell.self)
    collectionView.register(RecentSearchCell.self)
    collectionView.register(FavoritesHeaderView.self, forSupplementaryViewOfKind: UICollectionView.elementKindSectionHeader, withReuseIdentifier: "fav_header")
    collectionView.register(RecentSearchHeaderView.self, forSupplementaryViewOfKind: UICollectionView.elementKindSectionHeader, withReuseIdentifier: "recent_searches_header")

    favoritesFRC.delegate = self
    recentSearchesFRC.delegate = self
    hasPasteboardURL = UIPasteboard.general.hasStrings || UIPasteboard.general.hasURLs

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

    view.addSubview(backgroundView)
    view.addSubview(collectionView)

    backgroundView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    collectionView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    collectionView.alwaysBounceVertical = true
    collectionView.contentInset = UIEdgeInsets(top: 24, left: 0, bottom: 0, right: 0)
    collectionView.backgroundColor = .clear
    collectionView.dataSource = dataSource
    collectionView.delegate = self
    collectionView.dragDelegate = self
    collectionView.dropDelegate = self

    // Drag should be enabled to rearrange favourite
    collectionView.dragInteractionEnabled = true
    collectionView.keyboardDismissMode = .interactive

    dataSource.supplementaryViewProvider = { [weak self] collectionView, kind, indexPath in
      self?.supplementaryViewProvider(collectionView: collectionView, kind: kind, indexPath: indexPath)
    }

    updateUIWithSnapshot()
    updateColors()
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
  
  private func updateColors() {
    let browserColors = privateBrowsingManager.browserColors
    // Have to apply a custom alpha here because UIVisualEffectView blurs come with their own tint
    backgroundView.contentView.backgroundColor = browserColors.containerFrostedGlass.withAlphaComponent(0.8)
  }

  private func calculateAppropriateGrid() {
    let width = collectionView.bounds.width - (layout.sectionInset.left + layout.sectionInset.right) - (collectionView.contentInset.left + collectionView.contentInset.right)
    // Want to fit _at least_ 4 on all devices, but on larger devices
    // allowing the cells to be a bit bigger
    let minimumNumberOfColumns = 4
    let minWidth = floor(width / CGFloat(minimumNumberOfColumns))
    // Default width should be 82, but may get smaller or bigger
    var itemSize = CGSize(width: 82, height: FavoriteCell.height(forWidth: 82))
    if minWidth < 82 {
      itemSize = CGSize(width: floor(width / 4.0), height: FavoriteCell.height(forWidth: floor(width / 4.0)))
    } else if traitCollection.horizontalSizeClass == .regular {
      // On iPad's or Max/Plus phones allow the icons to get bigger to an
      // extent
      if width / CGFloat(minimumNumberOfColumns) > 100.0 {
        itemSize = CGSize(width: 100, height: FavoriteCell.height(forWidth: 100))
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

// MARK: - KeyboardHelperDelegate
extension FavoritesViewController: KeyboardHelperDelegate {
  func updateKeyboardInset(_ state: KeyboardState, animated: Bool = true) {
    if collectionView.bounds.size == .zero { return }
    let keyboardHeight = state.intersectionHeightForView(self.view) - view.safeAreaInsets.bottom + additionalSafeAreaInsets.bottom
    UIViewPropertyAnimator(duration: animated ? state.animationDuration : 0.0, curve: state.animationCurve) {
      self.collectionView.contentInset = self.collectionView.contentInset.with {
        $0.bottom = keyboardHeight
      }
      self.collectionView.scrollIndicatorInsets = self.collectionView.verticalScrollIndicatorInsets.with {
        $0.bottom = keyboardHeight
      }
    }.startAnimation()
  }

  func keyboardHelper(_ keyboardHelper: KeyboardHelper, keyboardWillShowWithState state: KeyboardState) {
    updateKeyboardInset(state)
  }

  func keyboardHelper(_ keyboardHelper: KeyboardHelper, keyboardWillHideWithState state: KeyboardState) {
    updateKeyboardInset(state)
  }

  private var availableSections: [Section] {
    var sections = [Section]()
    if hasPasteboardURL {
      sections.append(.pasteboard)
    }

    if let favoritesObjects = favoritesFRC.fetchedObjects, !favoritesObjects.isEmpty {
      sections.append(.favorites)
    }

    if !tabType.isPrivate && Preferences.Search.shouldShowRecentSearches.value, RecentSearch.totalCount() > 0 {
      sections.append(.recentSearches)
    } else if !tabType.isPrivate && Preferences.Search.shouldShowRecentSearchesOptIn.value {
      sections.append(.recentSearches)
    }
    return sections
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
    case .pasteboard:
      break
    case .favorites:
      guard let bookmark = favoritesFRC.fetchedObjects?[safe: indexPath.item] else {
        return
      }
      action(bookmark, .opened())
    case .recentSearches:
      guard let searchItem = recentSearchesFRC.fetchedObjects?[safe: indexPath.item] else {
        return
      }
      recentSearchAction(searchItem, true)
    }

  }

  func collectionView(_ collectionView: UICollectionView, targetIndexPathForMoveFromItemAt currentIndexPath: IndexPath, toProposedIndexPath proposedIndexPath: IndexPath) -> IndexPath {
    currentIndexPath.section == proposedIndexPath.section ? proposedIndexPath : currentIndexPath
  }

  func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, referenceSizeForHeaderInSection section: Int) -> CGSize {
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
      if Preferences.Search.shouldShowRecentSearches.value {
        return CGSize(width: collectionView.bounds.width, height: 22.0)
      }

      return CGSize(width: collectionView.bounds.width, height: 150.0)
    }
  }

  func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, sizeForItemAt indexPath: IndexPath) -> CGSize {
    guard let section = availableSections[safe: indexPath.section] else {
      assertionFailure("Invalid Section")
      return .zero
    }

    switch section {
    case .pasteboard:
      assertionFailure("Pasteboard section should have no items")
      return .zero
    case .favorites:
      return favoriteGridSize
    case .recentSearches:
      let width = collectionView.bounds.width - (layout.sectionInset.left + layout.sectionInset.right) - (collectionView.contentInset.left + collectionView.contentInset.right)
      return CGSize(width: width, height: 28.0)
    }
  }

  func collectionView(_ collectionView: UICollectionView, contextMenuConfigurationForItemAt indexPath: IndexPath, point: CGPoint) -> UIContextMenuConfiguration? {
    guard let section = availableSections[safe: indexPath.section] else {
      assertionFailure("Invalid Section")
      return nil
    }

    switch section {
    case .pasteboard:
      break
    case .favorites:
      guard let bookmark = favoritesFRC.fetchedObjects?[indexPath.item] else { return nil }
      return UIContextMenuConfiguration(identifier: indexPath as NSCopying, previewProvider: nil) { _ -> UIMenu? in
        let openInNewTab = UIAction(
          title: Strings.openNewTabButtonTitle,
          handler: UIAction.deferredActionHandler { _ in
            self.action(bookmark, .opened(inNewTab: true, switchingToPrivateMode: false))
          })
        let edit = UIAction(
          title: Strings.editFavorite,
          handler: UIAction.deferredActionHandler { _ in
            self.action(bookmark, .edited)
          })
        let delete = UIAction(
          title: Strings.removeFavorite, attributes: .destructive,
          handler: UIAction.deferredActionHandler { _ in
            bookmark.delete()
          })

        var urlChildren: [UIAction] = [openInNewTab]
        if !self.privateBrowsingManager.isPrivateBrowsing {
          let openInNewPrivateTab = UIAction(
            title: Strings.openNewPrivateTabButtonTitle,
            handler: UIAction.deferredActionHandler { _ in
              self.action(bookmark, .opened(inNewTab: true, switchingToPrivateMode: true))
            })
          urlChildren.append(openInNewPrivateTab)
        }

        let urlMenu = UIMenu(title: "", options: .displayInline, children: urlChildren)
        let favMenu = UIMenu(title: "", options: .displayInline, children: [edit, delete])
        return UIMenu(title: bookmark.title ?? bookmark.url ?? "", identifier: nil, children: [urlMenu, favMenu])
      }
    case .recentSearches:
      break
    }
    return nil
  }

  func collectionView(_ collectionView: UICollectionView, previewForHighlightingContextMenuWithConfiguration configuration: UIContextMenuConfiguration) -> UITargetedPreview? {
    guard let indexPath = configuration.identifier as? IndexPath,
      let cell = collectionView.cellForItem(at: indexPath) as? FavoriteCell
    else {
      return nil
    }
    return UITargetedPreview(view: cell.imageView)
  }

  func collectionView(_ collectionView: UICollectionView, previewForDismissingContextMenuWithConfiguration configuration: UIContextMenuConfiguration) -> UITargetedPreview? {
    guard let indexPath = configuration.identifier as? IndexPath,
      let cell = collectionView.cellForItem(at: indexPath) as? FavoriteCell
    else {
      return nil
    }
    return UITargetedPreview(view: cell.imageView)
  }
}

// MARK: - UICollectionViewDragDelegate & UICollectionViewDropDelegate
extension FavoritesViewController: UICollectionViewDragDelegate, UICollectionViewDropDelegate {
  func collectionView(_ collectionView: UICollectionView, itemsForBeginning session: UIDragSession, at indexPath: IndexPath) -> [UIDragItem] {
    guard let section = availableSections[safe: indexPath.section] else {
      assertionFailure("Invalid Section")
      return []
    }

    switch section {
    case .pasteboard:
      break
    case .favorites:
      // Fetch results controller indexpath is independent from our collection view.
      // All results of it are stored in first section.
      let adjustedIndexPath = IndexPath(row: indexPath.row, section: 0)
      let bookmark = favoritesFRC.object(at: adjustedIndexPath)
      let itemProvider = NSItemProvider(object: "\(indexPath)" as NSString)
      let dragItem = UIDragItem(itemProvider: itemProvider)
      dragItem.previewProvider = { () -> UIDragPreview? in
        guard let cell = collectionView.cellForItem(at: indexPath) as? FavoriteCell else {
          return nil
        }
        return UIDragPreview(view: cell.imageView)
      }
      dragItem.localObject = bookmark
      return [dragItem]
    case .recentSearches:
      break
    }
    return []
  }

  func collectionView(_ collectionView: UICollectionView, performDropWith coordinator: UICollectionViewDropCoordinator) {
    guard let sourceIndexPath = coordinator.items.first?.sourceIndexPath else { return }
    let destinationIndexPath: IndexPath
    if let indexPath = coordinator.destinationIndexPath {
      destinationIndexPath = indexPath
    } else {
      let section = max(collectionView.numberOfSections - 1, 0)
      let row = collectionView.numberOfItems(inSection: section)
      destinationIndexPath = IndexPath(row: max(row - 1, 0), section: section)
    }

    if sourceIndexPath.section != destinationIndexPath.section {
      return
    }

    switch coordinator.proposal.operation {
    case .move:
      guard let item = coordinator.items.first else { return }
      _ = coordinator.drop(item.dragItem, toItemAt: destinationIndexPath)
      Favorite.reorder(
        sourceIndexPath: sourceIndexPath,
        destinationIndexPath: destinationIndexPath,
        isInteractiveDragReorder: true
      )
    case .copy:
      break
    default: return
    }
  }

  func collectionView(_ collectionView: UICollectionView, dropSessionDidUpdate session: UIDropSession, withDestinationIndexPath destinationIndexPath: IndexPath?) -> UICollectionViewDropProposal {
    if favoritesFRC.fetchedObjects?.count == 1 {
      return .init(operation: .cancel)
    }
    return .init(operation: .move, intent: .insertAtDestinationIndexPath)
  }

  func collectionView(_ collectionView: UICollectionView, dragPreviewParametersForItemAt indexPath: IndexPath) -> UIDragPreviewParameters? {
    let params = UIDragPreviewParameters()
    params.backgroundColor = .clear
    if let cell = collectionView.cellForItem(at: indexPath) as? FavoriteCell {
      params.visiblePath = UIBezierPath(roundedRect: cell.imageView.frame, cornerRadius: 8)
    }
    return params
  }

  func collectionView(_ collectionView: UICollectionView, dropPreviewParametersForItemAt indexPath: IndexPath) -> UIDragPreviewParameters? {
    let params = UIDragPreviewParameters()
    params.backgroundColor = .clear
    if let cell = collectionView.cellForItem(at: indexPath) as? FavoriteCell {
      params.visiblePath = UIBezierPath(roundedRect: cell.imageView.frame, cornerRadius: 8)
    }
    return params
  }

  func collectionView(_ collectionView: UICollectionView, dragSessionIsRestrictedToDraggingApplication session: UIDragSession) -> Bool {
    return true
  }
}

// Recent Searches
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
    if Preferences.Search.shouldShowRecentSearches.value {
      // User already had recent searches enabled, and they want to see more results
      NSFetchedResultsController<RecentSearch>.deleteCache(withName: recentSearchesFRC.cacheName)
      recentSearchesFRC.fetchRequest.fetchLimit = 0
      fetchRecentSearches()
    } else {
      // User enabled recent searches
      Preferences.Search.shouldShowRecentSearches.value = true
      Preferences.Search.shouldShowRecentSearchesOptIn.value = false
    }
  }

  @objc
  func onRecentSearchHideOrClearPressed(_ button: UIButton) {
    if Preferences.Search.shouldShowRecentSearches.value {
      // User cleared recent searches

      // brave-ios/issues/3762
      // No title, no message
      let alert = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
      alert.addAction(
        UIAlertAction(
          title: Strings.recentSearchClearAlertButton,
          style: .default,
          handler: { _ in
            RecentSearch.removeAll()
          }))

      alert.popoverPresentationController?.sourceView = button
      alert.popoverPresentationController?.permittedArrowDirections = [.down, .up]

      alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .destructive, handler: nil))
      present(alert, animated: true, completion: nil)
    } else {
      // User doesn't want to see the recent searches option again
      Preferences.Search.shouldShowRecentSearchesOptIn.value = false
    }
  }
}

extension FavoritesViewController: PreferencesObserver {
  func preferencesDidChange(for key: String) {
    updateUIWithSnapshot()
  }
}

// MARK: - Diffable data source + NSFetchedResultsControllerDelegate
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

    let fetchedFavorites: [DataWrapper]? = favoritesFRC.fetchedObjects?.compactMap {
      return .favorite(FavoriteDiffable(objectID: $0.objectID, title: $0.displayTitle, url: $0.url))
    }

    if favoritesSectionExists, let favorites = fetchedFavorites {
      snapshot.appendItems(favorites, toSection: .favorites)
    }

    if recentSearchesSectionExists, let objects = recentSearchesFRC.fetchedObjects {
      snapshot.appendItems(
        objects.compactMap({ .recentSearch($0.objectID) }),
        toSection: .recentSearches)
    }

    dataSource.apply(snapshot, animatingDifferences: false)
  }

  func controller(
    _ controller: NSFetchedResultsController<NSFetchRequestResult>,
    didChangeContentWith snapshot: NSDiffableDataSourceSnapshotReference
  ) {

    // Skip applying snapshots before first `initialSnapshotSetup` is made.
    if dataSource.snapshot().numberOfSections == 0 {
      return
    }

    let currentSnapshot = dataSource.snapshot()
    var newSnapshot = Snapshot()
    newSnapshot.appendSections(availableSections)

    guard let ids = snapshot.itemIdentifiers as? [NSManagedObjectID] else { return }

    if controller === favoritesFRC {
      var items = [DataWrapper]()

      if favoritesSectionExists {
        ids.forEach {
          // Fetch existing item from the DB then add it to snapshot.
          // This way the snapshot will be able to detect changes on the object.
          // Non existing objects are not added, this simulates removing the item.
          if let existingItem = controller.managedObjectContext.object(with: $0) as? Favorite {
            items.append(
              .favorite(
                FavoriteDiffable(
                  objectID: $0,
                  title: existingItem.title,
                  url: existingItem.url)))
          }
        }

        newSnapshot.appendItems(items, toSection: .favorites)
      }

      // New snapshot is created, items from the other frc must be added to it.
      if recentSearchesSectionExists {
        newSnapshot.appendItems(currentSnapshot.itemIdentifiers(inSection: .recentSearches), toSection: .recentSearches)
      }
    }

    if controller === recentSearchesFRC {
      if favoritesSectionExists {
        // New snapshot is created, items from the other frc must be added to it.
        newSnapshot.appendItems(
          currentSnapshot.itemIdentifiers(inSection: .favorites),
          toSection: .favorites)
      }

      if recentSearchesSectionExists {
        var items = [DataWrapper]()

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
    wrapper: DataWrapper
  ) -> UICollectionViewCell? {

    switch wrapper {
    case .favorite(let favoriteWrapper):
      guard let favorite = Favorite.get(with: favoriteWrapper.objectID) else { return nil }

      let cell = collectionView.dequeueReusableCell(for: indexPath) as FavoriteCell

      cell.textLabel.text = favorite.displayTitle ?? favorite.url
      if let url = favorite.url?.asURL {
        cell.imageView.loadFavicon(siteURL: url, isPrivateBrowsing: self.privateBrowsingManager.isPrivateBrowsing)
      }
      cell.accessibilityLabel = cell.textLabel.text

      return cell
    case .recentSearch(let objectID):
      guard let recentSearch = RecentSearch.get(with: objectID) else { return nil }

      let cell = collectionView.dequeueReusableCell(for: indexPath) as RecentSearchCell

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
            attributes: [.font: UIFont.systemFont(ofSize: 15.0, weight: .semibold)])
          title.append(
            NSAttributedString(
              string: "\"\(text)\"",
              attributes: [.font: UIFont.systemFont(ofSize: 15.0)]))
          cell.setAttributedTitle(title)
        }
      case .website:
        if let text = recentSearch.text,
          let websiteUrl = recentSearch.websiteUrl {
          let website = URL(string: websiteUrl)?.baseDomain ?? URL(string: websiteUrl)?.host ?? websiteUrl

          let title = NSMutableAttributedString(
            string: text,
            attributes: [.font: UIFont.systemFont(ofSize: 15.0)])
          title.append(
            NSAttributedString(
              string: " \(Strings.recentSearchQuickSearchOnWebsite) ",
              attributes: [.font: UIFont.systemFont(ofSize: 15.0, weight: .semibold)]))
          title.append(
            NSAttributedString(
              string: website,
              attributes: [.font: UIFont.systemFont(ofSize: 15.0)]))
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

  private func supplementaryViewProvider(collectionView: UICollectionView, kind: String, indexPath: IndexPath) -> UICollectionReusableView? {
    guard let section = availableSections[safe: indexPath.section] else {
      assertionFailure("Invalid Section")
      return UICollectionReusableView()
    }

    if kind == UICollectionView.elementKindSectionHeader {
      switch section {
      case .pasteboard:
        if let header = collectionView.dequeueReusableSupplementaryView(ofKind: kind, withReuseIdentifier: "pasteboard_header", for: indexPath) as? RecentSearchClipboardHeaderView {
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
            for: indexPath)
      case .recentSearches:
        if let header = collectionView.dequeueReusableSupplementaryView(ofKind: kind, withReuseIdentifier: "recent_searches_header", for: indexPath) as? RecentSearchHeaderView {
          header.resetLayout(showRecentSearches: Preferences.Search.shouldShowRecentSearches.value)

          header.showButton.addTarget(self, action: #selector(onRecentSearchShowPressed), for: .touchUpInside)
          header.hideClearButton.addTarget(self, action: #selector(onRecentSearchHideOrClearPressed(_:)), for: .touchUpInside)

          let shouldShowRecentSearches = Preferences.Search.shouldShowRecentSearches.value
          var showButtonVisible = !shouldShowRecentSearches
          var clearButtonVisible = !shouldShowRecentSearches
          if let fetchedObjects = recentSearchesFRC.fetchedObjects, shouldShowRecentSearches {
            let totalCount = RecentSearch.totalCount()
            showButtonVisible = fetchedObjects.count < totalCount
            clearButtonVisible = fetchedObjects.count <= totalCount
          }
          header.setButtonVisibility(
            showButtonVisible: showButtonVisible,
            clearButtonVisible: clearButtonVisible
          )

          return header
        }
      }

    }
    return UICollectionReusableView()
  }
}
