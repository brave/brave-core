/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Storage
import CoreData
import Shared
import Data

private let log = Logger.browserLogger

class FavoritesDataSource: NSObject, UICollectionViewDataSource {
    var frc: NSFetchedResultsController<Bookmark>?
    weak var collectionView: UICollectionView?
    
    var favoriteDeletedHandler: (() -> Void)?

    var isEditing: Bool = false {
        didSet {
            if isEditing != oldValue {
                // We need to post notification here to inform all cells to show the edit button.
                // collectionView.reloadData() can't be used, it stops InteractiveMovementForItem,
                // requiring user to long press again if he wants to reorder a tile.
                let name = isEditing ? Notification.Name.thumbnailEditOn : Notification.Name.thumbnailEditOff
                NotificationCenter.default.post(name: name, object: nil)
            }
        }
    }

    override init() {
        super.init()

        frc = Bookmark.frc(forFavorites: true, parentFolder: nil)
        frc?.delegate = self

        do {
            try frc?.performFetch()
        } catch {
            log.error("Favorites fetch error")
        }
    }
    
    /// The number of times that each row contains
    var columnsPerRow: Int {
        guard let size = collectionView?.bounds.size,
            let traitCollection = collectionView?.traitCollection else {
                return 0
        }
        
        var cols = 0
        if traitCollection.horizontalSizeClass == .compact {
            // Landscape iPhone
            if traitCollection.verticalSizeClass == .compact {
                cols = 5
            }
                // Split screen iPad width
            else if size.widthLargerOrEqualThanHalfIPad() {
                cols = 4
            }
                // iPhone portrait
            else {
                cols = 3
            }
        } else {
            // Portrait iPad
            if size.height > size.width {
                cols = 4
            }
                // Landscape iPad
            else {
                cols = 5
            }
        }
        return cols + 1
    }
    
    /// If there are more favorites than are being shown
    var hasOverflow: Bool {
        return columnsPerRow < frc?.fetchedObjects?.count ?? 0
    }
    
    func refetch() {
        try? frc?.performFetch()
    }
    
    func favoriteBookmark(at indexPath: IndexPath) -> Bookmark? {
        // Favorites may be not updated at this point, fetching them again.
        try? frc?.performFetch()
        return frc?.object(at: indexPath)
    }

    func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
        return min(columnsPerRow, frc?.fetchedObjects?.count ?? 0)
    }

    func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
        // swiftlint:disable:next force_cast
        let cell = collectionView.dequeueReusableCell(withReuseIdentifier: FavoriteCell.identifier, for: indexPath) as! FavoriteCell
        return configureCell(cell: cell, at: indexPath)
    }

    func collectionView(_ collectionView: UICollectionView, moveItemAt sourceIndexPath: IndexPath, to destinationIndexPath: IndexPath) {
        // Using the same reorder logic as in BookmarksPanel
        Bookmark.reorderBookmarks(frc: frc, sourceIndexPath: sourceIndexPath, destinationIndexPath: destinationIndexPath)
    }

    @discardableResult
    fileprivate func configureCell(cell: FavoriteCell, at indexPath: IndexPath) -> UICollectionViewCell {
        guard let fav = frc?.object(at: indexPath) else { return UICollectionViewCell() }

        cell.textLabel.text = fav.displayTitle ?? fav.url
        cell.imageView.setIconMO(nil, forURL: URL(string: fav.url ?? ""), scaledDefaultIconSize: CGSize(width: 40, height: 40), completed: { (color, url) in
            if fav.url == url?.absoluteString {
                cell.imageView.backgroundColor = color
            }
        })
        cell.accessibilityLabel = cell.textLabel.text

        cell.toggleEditButton(isEditing)
        
        return cell
    }
}

extension FavoritesDataSource: NSFetchedResultsControllerDelegate {
    func controller(_ controller: NSFetchedResultsController<NSFetchRequestResult>, didChange anObject: Any, at indexPath: IndexPath?, for type: NSFetchedResultsChangeType, newIndexPath: IndexPath?) {

        switch type {
        case .insert:
            if let indexPath = newIndexPath {
                collectionView?.insertItems(at: [indexPath])
            }
        case .delete:
            // Not all favorites must be visible at the time, so we can't just call `deleteItems` here.
            // Example:
            // There's 10 favorites total, 4 are visible on iPhone.
            // We delete second favorite(index=1), visible favorites count should still be 4.
            // Favorites from places 3 and 4 are moved back to 2 and 3
            // a new, previously hidden favorite item is now shown at position 4.
            collectionView?.reloadData()
            favoriteDeletedHandler?()
        case .update:
            if let indexPath = indexPath, let cell = collectionView?.cellForItem(at: indexPath) as? FavoriteCell {
                configureCell(cell: cell, at: indexPath)
            }
            if let newIndexPath = newIndexPath, let cell = collectionView?.cellForItem(at: newIndexPath) as? FavoriteCell {
                configureCell(cell: cell, at: newIndexPath)
            }
        case .move:
            break
        @unknown default:
            assertionFailure()
            break
        }
    }
}
