// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Data
import Shared
import BraveShared
import BraveUI
import CoreData

private let log = Logger.browserLogger

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
    private let backgroundView = UIVisualEffectView(effect: UIBlurEffect(style: .systemChromeMaterial)).then {
        $0.contentView.backgroundColor = UIColor.braveBackground.withAlphaComponent(0.5)
    }
    private var hasPasteboardURL = false
    
    init(tabType: TabType, action: @escaping (Favorite, BookmarksAction) -> Void, recentSearchAction: @escaping (RecentSearch?, Bool) -> Void) {
        self.tabType = tabType
        self.action = action
        self.recentSearchAction = recentSearchAction
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
        
        do {
            try favoritesFRC.performFetch()
        } catch {
            log.error("Favorites fetch error: \(String(describing: error))")
        }
        
        fetchRecentSearches()
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
        collectionView.dataSource = self
        collectionView.delegate = self
        collectionView.dragDelegate = self
        collectionView.dropDelegate = self
        collectionView.dragInteractionEnabled = true
        collectionView.keyboardDismissMode = .interactive
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
    
    private func calculateAppropriateGrid() {
        let width = collectionView.bounds.width -
            (layout.sectionInset.left + layout.sectionInset.right) -
            (collectionView.contentInset.left + collectionView.contentInset.right)
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
                log.error("Recent Searches fetch error: \(String(describing: error))")
            }
        }
    }
    
    private var frcOperations: [BlockOperation] = []
}

// MARK: - KeyboardHelperDelegate
extension FavoritesViewController: KeyboardHelperDelegate {
    func updateKeyboardInset(_ state: KeyboardState, animated: Bool = true) {
        if collectionView.bounds.size == .zero { return }
        let keyboardHeight = state.intersectionHeightForView(self.view) - view.safeAreaInsets.bottom
        UIView.animate(withDuration: animated ? state.animationDuration : 0.0, animations: {
            if animated {
                UIView.setAnimationCurve(state.animationCurve)
            }
            self.collectionView.contentInset = self.collectionView.contentInset.with {
                $0.bottom = keyboardHeight
            }
            self.collectionView.scrollIndicatorInsets = self.collectionView.scrollIndicatorInsets.with {
                $0.bottom = keyboardHeight
            }
        })
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
        
        sections.append(.favorites)
        
        if !tabType.isPrivate &&
           Preferences.Search.shouldShowRecentSearches.value &&
            recentSearchesFRC.fetchedObjects?.isEmpty == false {
            sections.append(.recentSearches)
        } else if !tabType.isPrivate && Preferences.Search.shouldShowRecentSearchesOptIn.value {
            sections.append(.recentSearches)
        }
        return sections
    }
}

// MARK: - UICollectionViewDataSource & UICollectionViewDelegateFlowLayout
extension FavoritesViewController: UICollectionViewDataSource, UICollectionViewDelegateFlowLayout {
    func numberOfSections(in collectionView: UICollectionView) -> Int {
        return availableSections.count
    }
    
    func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
        guard let section = availableSections[safe: section] else {
            assertionFailure("Invalid Section")
            return 0
        }
        
        switch section {
        case .pasteboard:
            return 0
        case .favorites:
            return favoritesFRC.fetchedObjects?.count ?? 0
        case .recentSearches:
            if !tabType.isPrivate && Preferences.Search.shouldShowRecentSearches.value {
                return recentSearchesFRC.fetchedObjects?.count ?? 0
            }
            return 0
        }
    }
    
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
    
    func collectionView(_ collectionView: UICollectionView, viewForSupplementaryElementOfKind kind: String, at indexPath: IndexPath) -> UICollectionReusableView {
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
                return collectionView.dequeueReusableSupplementaryView(ofKind: kind, withReuseIdentifier: "fav_header", for: indexPath)
            case .recentSearches:
                if let header = collectionView.dequeueReusableSupplementaryView(ofKind: kind, withReuseIdentifier: "recent_searches_header", for: indexPath) as? RecentSearchHeaderView {
                    header.resetLayout(showRecentSearches: Preferences.Search.shouldShowRecentSearches.value)
                    header.showButton.removeTarget(self, action: nil, for: .touchUpInside)
                    header.hideClearButton.removeTarget(self, action: nil, for: .touchUpInside)
                    
                    header.showButton.addTarget(self, action: #selector(onRecentSearchShowPressed), for: .touchUpInside)
                    header.hideClearButton.addTarget(self, action: #selector(onRecentSearchHideOrClearPressed), for: .touchUpInside)
                    
                    if Preferences.Search.shouldShowRecentSearches.value {
                        let totalCount = RecentSearch.totalCount()
                        if let fetchedObjects = recentSearchesFRC.fetchedObjects {
                            if fetchedObjects.count < totalCount {
                                header.setButtonVisibility(showButtonVisible: true, clearButtonVisible: true)
                            } else if fetchedObjects.count == totalCount {
                                header.setButtonVisibility(showButtonVisible: false, clearButtonVisible: true)
                            } else {
                                header.setButtonVisibility(showButtonVisible: false, clearButtonVisible: false)
                            }
                        } else {
                            header.setButtonVisibility(showButtonVisible: false, clearButtonVisible: false)
                        }
                    } else {
                        header.setButtonVisibility(showButtonVisible: true, clearButtonVisible: true)
                    }
                    return header
                }
            }
            
        }
        return UICollectionReusableView()
    }
    
    func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
        guard let section = availableSections[safe: indexPath.section] else {
            assertionFailure("Invalid Section")
            return UICollectionViewCell()
        }
        
        switch section {
        case .pasteboard:
            assertionFailure("Pasteboard section should have no items")
            return UICollectionViewCell()
        case .favorites:
            let cell = collectionView.dequeueReusableCell(for: indexPath) as FavoriteCell
            let fav = favoritesFRC.object(at: IndexPath(item: indexPath.item, section: 0))
            cell.textLabel.text = fav.displayTitle ?? fav.url
            if let url = fav.url?.asURL {
                // All favorites should have domain's, but it was noticed at one
                // point that this wasn't the case, so for future bug-tracking
                // assert if its not found.
                assert(fav.domain != nil, "Domain should exist for all favorites")
                // The domain for the favorite is required for pulling cached
                // favicon info. Since all favorites should have persisted
                // Domain's, we leave `persistent` as true
                let domain = fav.domain ?? Domain.getOrCreate(forUrl: url, persistent: true)
                cell.imageView.loadFavicon(siteURL: url, domain: domain, monogramFallbackCharacter: fav.title?.first)
            }
            cell.accessibilityLabel = cell.textLabel.text
            return cell
            
        case .recentSearches:
            let cell = collectionView.dequeueReusableCell(for: indexPath) as RecentSearchCell
            let recentSearch = recentSearchesFRC.object(at: IndexPath(item: indexPath.item, section: 0))
            guard let searchType = RecentSearchType(rawValue: recentSearch.searchType) else {
                cell.setTitle(recentSearch.text)
                return cell
            }
            
            cell.openButtonAction = { [unowned self] in
                self.onOpenRecentSearch(recentSearch)
            }
            
            switch searchType {
            case .text:
                cell.setTitle(recentSearch.text)
            case .qrCode:
                if let text = recentSearch.text ?? recentSearch.websiteUrl {
                    let title = NSMutableAttributedString(string: "\(Strings.recentSearchScanned) ",
                                                          attributes: [.font: UIFont.systemFont(ofSize: 15.0, weight: .semibold)])
                    title.append(NSAttributedString(string: "\"\(text)\"",
                                                    attributes: [.font: UIFont.systemFont(ofSize: 15.0)]))
                    cell.setAttributedTitle(title)
                }
            case .website:
                if let text = recentSearch.text,
                   let websiteUrl = recentSearch.websiteUrl {
                    let website = URL(string: websiteUrl)?.baseDomain ?? URL(string: websiteUrl)?.host ?? websiteUrl
                    
                    let title = NSMutableAttributedString(string: text,
                                                          attributes: [.font: UIFont.systemFont(ofSize: 15.0)])
                    title.append(NSAttributedString(string: " \(Strings.recentSearchQuickSearchOnWebsite) ",
                                                    attributes: [.font: UIFont.systemFont(ofSize: 15.0, weight: .semibold)]))
                    title.append(NSAttributedString(string: website,
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
            let width = collectionView.bounds.width -
                (layout.sectionInset.left + layout.sectionInset.right) -
                (collectionView.contentInset.left + collectionView.contentInset.right)
            return CGSize(width: width, height: 28.0)
        }
    }
    
    func collectionView(_ collectionView: UICollectionView, canMoveItemAt indexPath: IndexPath) -> Bool {
        guard let section = availableSections[safe: indexPath.section] else {
            assertionFailure("Invalid Section")
            return false
        }
        
        switch section {
        case .pasteboard:
            return false
        case .favorites:
            return true
        case .recentSearches:
            return false
        }
    }
    
    func collectionView(_ collectionView: UICollectionView, moveItemAt sourceIndexPath: IndexPath, to destinationIndexPath: IndexPath) {
        guard let section = availableSections[safe: sourceIndexPath.section] else {
            assertionFailure("Invalid Section")
            return
        }
        
        switch section {
        case .pasteboard:
            break
        case .favorites:
            Favorite.reorder(sourceIndexPath: sourceIndexPath, destinationIndexPath: destinationIndexPath)
        case .recentSearches:
            break
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
                let openInNewTab = UIAction(title: Strings.openNewTabButtonTitle, handler: UIAction.deferredActionHandler { _ in
                    self.action(bookmark, .opened(inNewTab: true, switchingToPrivateMode: false))
                })
                let edit = UIAction(title: Strings.editBookmark, handler: UIAction.deferredActionHandler { _ in
                    self.action(bookmark, .edited)
                })
                let delete = UIAction(title: Strings.removeFavorite, attributes: .destructive, handler: UIAction.deferredActionHandler { _ in
                    bookmark.delete()
                })
                
                var urlChildren: [UIAction] = [openInNewTab]
                if !PrivateBrowsingManager.shared.isPrivateBrowsing {
                    let openInNewPrivateTab = UIAction(title: Strings.openNewPrivateTabButtonTitle, handler: UIAction.deferredActionHandler { _ in
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
            let cell = collectionView.cellForItem(at: indexPath) as? FavoriteCell else {
                return nil
        }
        return UITargetedPreview(view: cell.imageView)
    }
    
    func collectionView(_ collectionView: UICollectionView, previewForDismissingContextMenuWithConfiguration configuration: UIContextMenuConfiguration) -> UITargetedPreview? {
        guard let indexPath = configuration.identifier as? IndexPath,
            let cell = collectionView.cellForItem(at: indexPath) as? FavoriteCell else {
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
            let bookmark = favoritesFRC.object(at: indexPath)
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
            let section = collectionView.numberOfSections - 1
            let row = collectionView.numberOfItems(inSection: section)
            destinationIndexPath = IndexPath(row: row - 1, section: section)
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
            try? favoritesFRC.performFetch()
            collectionView.moveItem(at: sourceIndexPath, to: destinationIndexPath)
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

// MARK: - NSFetchedResultsControllerDelegate
extension FavoritesViewController: NSFetchedResultsControllerDelegate {
    func controllerWillChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
        if collectionView.hasActiveDrag || collectionView.hasActiveDrop { return }
        frcOperations.removeAll()
    }
    
    func controller(_ controller: NSFetchedResultsController<NSFetchRequestResult>, didChange anObject: Any, at indexPath: IndexPath?, for type: NSFetchedResultsChangeType, newIndexPath: IndexPath?) {
        
        var indexPath = indexPath
        var newIndexPath = newIndexPath
        if controller == favoritesFRC {
            indexPath?.section = Section.favorites.rawValue
            newIndexPath?.section = Section.favorites.rawValue
        } else if controller == recentSearchesFRC {
            return
        } else {
            assertionFailure("Invalid Section")
        }
        
        if collectionView.hasActiveDrag || collectionView.hasActiveDrop { return }
        switch type {
        case .insert:
            if let newIndexPath = newIndexPath {
                frcOperations.append(BlockOperation { [weak self] in
                    self?.collectionView.insertItems(at: [newIndexPath])
                })
            }
        case .delete:
            if let indexPath = indexPath {
                frcOperations.append(BlockOperation { [weak self] in
                    self?.collectionView.deleteItems(at: [indexPath])
                })
            }
        case .update:
            if let indexPath = indexPath {
                frcOperations.append(BlockOperation { [weak self] in
                    self?.collectionView.reloadItems(at: [indexPath])
                })
            }
            if let newIndexPath = newIndexPath, newIndexPath != indexPath {
                frcOperations.append(BlockOperation { [weak self] in
                    self?.collectionView.reloadItems(at: [newIndexPath])
                })
            }
        case .move:
            break
        @unknown default:
            assertionFailure()
        }
    }
    
    func controllerDidChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
        if collectionView.hasActiveDrag || collectionView.hasActiveDrop { return }
        if controller == recentSearchesFRC {
            self.collectionView.reloadData()
            return
        }
        
        collectionView.performBatchUpdates({
            self.frcOperations.forEach {
                $0.start()
            }
        }, completion: { _ in
            self.frcOperations.removeAll()
            self.collectionView.reloadData()
        })
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
            collectionView.reloadData()
        } else {
            // User enabled recent searches
            Preferences.Search.shouldShowRecentSearches.value = true
            Preferences.Search.shouldShowRecentSearchesOptIn.value = false
            fetchRecentSearches()
            collectionView.reloadData()
        }
    }
    
    @objc
    func onRecentSearchHideOrClearPressed() {
        if Preferences.Search.shouldShowRecentSearches.value {
            // User cleared recent searches
            
            // brave-ios/issues/3762
            // No title, no message
            let alert = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
            alert.addAction(UIAlertAction(title: Strings.recentSearchClearAlertButton, style: .default, handler: { [weak self] _ in
                guard let self = self else { return }
                
                RecentSearch.removeAll()
                self.fetchRecentSearches()
                self.collectionView.reloadData()
            }))
            
            alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .destructive, handler: nil))
            present(alert, animated: true, completion: nil)
        } else {
            // User doesn't want to see the recent searches option again
            Preferences.Search.shouldShowRecentSearchesOptIn.value = false
            collectionView.reloadData()
        }
    }
}
