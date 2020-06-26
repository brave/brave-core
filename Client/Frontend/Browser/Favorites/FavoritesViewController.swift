// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Data
import Shared
import CoreData

private let log = Logger.browserLogger

private class FavoritesHeaderView: UICollectionReusableView {
    let label = UILabel().then {
        $0.text = "Favorites"
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

class FavoritesViewController: UIViewController, Themeable {
    
    var action: (Bookmark, BookmarksAction) -> Void
    
    private let frc = Bookmark.frc(forFavorites: true, parentFolder: nil)
    
    private let layout = UICollectionViewFlowLayout().then {
        $0.sectionInset = UIEdgeInsets(top: 12, left: 0, bottom: 12, right: 0)
        $0.minimumInteritemSpacing = 0
        $0.minimumLineSpacing = 8
    }
    private let collectionView: UICollectionView
    private let backgroundView = UIVisualEffectView()
    
    init(action: @escaping (Bookmark, BookmarksAction) -> Void) {
        self.action = action
        collectionView = UICollectionView(frame: .zero, collectionViewLayout: layout)
        
        super.init(nibName: nil, bundle: nil)
        
        collectionView.register(FavoriteCell.self, forCellWithReuseIdentifier: FavoriteCell.identifier)
        collectionView.register(FavoritesHeaderView.self, forSupplementaryViewOfKind: UICollectionView.elementKindSectionHeader, withReuseIdentifier: "header")
        
        frc.delegate = self
        
        KeyboardHelper.defaultHelper.addDelegate(self)
        
        do {
            try frc.performFetch()
        } catch {
            log.error("Favorites fetch error: \(String(describing: error))")
        }
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
    
    func applyTheme(_ theme: Theme) {
        let blurStyle: UIBlurEffect.Style = theme.isDark ? .dark : .extraLight
        backgroundView.effect = UIBlurEffect(style: blurStyle)
        backgroundView.contentView.backgroundColor = theme.colors.home.withAlphaComponent(0.5)
        collectionView.reloadSections(IndexSet(integer: 0))
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
        layout.itemSize = itemSize
        layout.invalidateLayout()
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
    
    func keyboardHelper(_ keyboardHelper: KeyboardHelper, keyboardDidShowWithState state: KeyboardState) {
    }
    
    func keyboardHelper(_ keyboardHelper: KeyboardHelper, keyboardWillHideWithState state: KeyboardState) {
        updateKeyboardInset(state)
    }
}

// MARK: - UICollectionViewDataSource & UICollectionViewDelegateFlowLayout
extension FavoritesViewController: UICollectionViewDataSource, UICollectionViewDelegateFlowLayout {
    func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
        return frc.fetchedObjects?.count ?? 0
    }
    
    func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
        guard let bookmark = frc.fetchedObjects?[safe: indexPath.item] else {
            return
        }
        action(bookmark, .opened())
    }
    
    func collectionView(_ collectionView: UICollectionView, viewForSupplementaryElementOfKind kind: String, at indexPath: IndexPath) -> UICollectionReusableView {
        if kind == UICollectionView.elementKindSectionHeader {
            return collectionView.dequeueReusableSupplementaryView(ofKind: kind, withReuseIdentifier: "header", for: indexPath)
        }
        return UICollectionReusableView()
    }
    
    func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
        // swiftlint:disable:next force_cast
        let cell = collectionView.dequeueReusableCell(withReuseIdentifier: FavoriteCell.identifier, for: indexPath) as! FavoriteCell
        let fav = frc.object(at: IndexPath(item: indexPath.item, section: 0))
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
            cell.imageView.monogramFallbackCharacter = fav.title?.first
            cell.imageView.domain = domain
        }
        cell.accessibilityLabel = cell.textLabel.text
        cell.longPressHandler = { [weak self] cell in
            guard let self = self else { return }
            let alert = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
            
            let edit = UIAlertAction(title: Strings.editBookmark, style: .default) { (action) in
                self.action(fav, .edited)
            }
            let delete = UIAlertAction(title: Strings.removeFavorite, style: .destructive) { (action) in
                fav.delete()
            }
            
            alert.addAction(edit)
            alert.addAction(delete)
            
            alert.popoverPresentationController?.sourceView = cell
            alert.popoverPresentationController?.permittedArrowDirections = [.down, .up]
            alert.addAction(UIAlertAction(title: Strings.close, style: .cancel, handler: nil))
            
            UIImpactFeedbackGenerator(style: .medium).bzzt()
            self.present(alert, animated: true)
        }
        return cell
    }
    
    func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, referenceSizeForHeaderInSection section: Int) -> CGSize {
        return CGSize(width: collectionView.bounds.width, height: 32)
    }
    
    func collectionView(_ collectionView: UICollectionView, canMoveItemAt indexPath: IndexPath) -> Bool {
        return true
    }
    
    func collectionView(_ collectionView: UICollectionView, moveItemAt sourceIndexPath: IndexPath, to destinationIndexPath: IndexPath) {
        Bookmark.reorderBookmarks(frc: frc, sourceIndexPath: sourceIndexPath, destinationIndexPath: destinationIndexPath)
    }
    
    @available(iOS 13, *)
    func collectionView(_ collectionView: UICollectionView, contextMenuConfigurationForItemAt indexPath: IndexPath, point: CGPoint) -> UIContextMenuConfiguration? {
        guard let bookmark = frc.fetchedObjects?[indexPath.item] else { return nil }
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
    }
    
    @available(iOS 13.0, *)
    func collectionView(_ collectionView: UICollectionView, previewForHighlightingContextMenuWithConfiguration configuration: UIContextMenuConfiguration) -> UITargetedPreview? {
        guard let indexPath = configuration.identifier as? IndexPath,
            let cell = collectionView.cellForItem(at: indexPath) as? FavoriteCell else {
                return nil
        }
        return UITargetedPreview(view: cell.imageView)
    }
    
    @available(iOS 13.0, *)
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
        let bookmark = frc.object(at: indexPath)
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
            Bookmark.reorderBookmarks(
                frc: frc,
                sourceIndexPath: sourceIndexPath,
                destinationIndexPath: destinationIndexPath,
                isInteractiveDragReorder: true
            )
            try? frc.performFetch()
            collectionView.moveItem(at: sourceIndexPath, to: destinationIndexPath)
        case .copy:
            break
        default: return
        }
    }
    
    func collectionView(_ collectionView: UICollectionView, dropSessionDidUpdate session: UIDropSession, withDestinationIndexPath destinationIndexPath: IndexPath?) -> UICollectionViewDropProposal {
        if frc.fetchedObjects?.count == 1 {
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
