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

    var isEditing: Bool = false {
        didSet {
            if isEditing != oldValue {
                // We need to post notification here to inform all cells to show the edit button.
                // collectionView.reloadData() can't be used, it stops InteractiveMovementForItem,
                // requiring user to long press again if he wants to reorder a tile.
                let name = isEditing ? Notification.Name.ThumbnailEditOn : Notification.Name.ThumbnailEditOff
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
    
    func refetch() {
        try? frc?.performFetch()
    }
    
    func favoriteBookmark(at indexPath: IndexPath) -> Bookmark? {
        // Favorites may be not updated at this point, fetching them again.
        try? frc?.performFetch()
        return frc?.object(at: indexPath)
    }

    func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
        return frc?.fetchedObjects?.count ?? 0
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
            if let indexPath = indexPath {
                collectionView?.deleteItems(at: [indexPath])
            }
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
