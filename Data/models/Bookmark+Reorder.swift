// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import CoreData
import Foundation
import Shared
import Storage

private let log = Logger.browserLogger

extension Bookmark {
    /// Reordering bookmarks has two steps:
    /// 1. Sets new `syncOrder` for the source(moving) Bookmark
    /// 2. Recalculates `syncOrder` for all Bookmarks on a given level. This is required because
    /// we use a special String-based order and algorithg. Simple String comparision doesn't work here.
    ///
    /// Passing in `isInteractiveDragReorder` will force the write to happen on
    /// the main view context. Defaults to `false`
    public class func reorderBookmarks(frc: NSFetchedResultsController<Bookmark>?, sourceIndexPath: IndexPath,
                                       destinationIndexPath: IndexPath,
                                       isInteractiveDragReorder: Bool = false) {
        guard let frc = frc else { return }
        
        let dest = frc.object(at: destinationIndexPath)
        let src = frc.object(at: sourceIndexPath)
        
        if dest === src {
            log.error("Source and destination bookmarks are the same!")
            return
        }
        
        // To avoid mixing threads, the FRC can't be used within background context operation.
        // We take data that relies on the FRC on main thread and pass it into background context
        // in a safely manner.
        guard let data = getReorderData(fromFrc: frc, sourceIndexPath: sourceIndexPath,
                                        destinationIndexPath: destinationIndexPath) else {
                                            log.error("""
                Failed to receive enough data from FetchedResultsController \
                to perform bookmark reordering.
                """)
                                            return
        }
        
        // If we're doing an interactive drag reorder then we want to make sure
        // to do the reorder on main queue so the underlying dataset & FRC is
        // updated immediately so the animation does not glitch out on drop.
        //
        // Doing it on a background thread will cause the favorites overlay
        // to temporarely show the old items and require a full-table refresh
        let context: WriteContext = isInteractiveDragReorder ?
            .existing(DataController.viewContext) :
            .new(inMemory: false)
        DataController.perform(context: context) { context in
            // To get reordering right, 3 Bookmark objects are needed:
            
            // 1. Source Bookmark - A Bookmark that we are moving with a drag operation
            // 2. Destination Bookmark - A Bookmark, which is a neighbour Bookmark depending on drag direction:
            // a) when moving from bottom to top, all other bookmarks are pushed up
            // and the destination bookmark is placed before the source Bookmark
            // b) when moving frop top to bottom, all other bookmarks are pushed down
            // and the destination bookmark is placed after the source Bookmark.
            guard let srcBookmark = context.bookmark(with: src.objectID),
                let destBookmark = context.bookmark(with: dest.objectID) else {
                    log.error("Could not retrieve source or destination bookmark on background context.")
                    return
            }
            
            // Third object is next or previous Bookmark, depending on drag direction.
            // This Bookmark can also be empty when the source Bookmark is moved to the top or bottom
            // of the Bookmark collection.
            var nextOrPreviousBookmark: Bookmark?
            if let previousObjectId = data.nextOrPreviousBookmarkId {
                nextOrPreviousBookmark = context.bookmark(with: previousObjectId)
            }
            
            // syncOrder should be always set. Even if Sync is not initiated, we use
            // defualt local ordering starting with syncOrder = 0.0.1, 0.0.2 and so on.
            guard let destinationBookmarkSyncOrder = destBookmark.syncOrder else {
                log.error("syncOrder of destination bookmark is nil.")
                return
            }
            
            var previousOrder: String?
            var nextOrder: String?
            
            switch data.reorderMovement {
            case .up(let toTheTop):
                // Going up pushes all bookmarks down, so the destination bookmark is after the source bookmark
                nextOrder = destinationBookmarkSyncOrder
                if !toTheTop {
                    guard let previousSyncOrder = nextOrPreviousBookmark?.syncOrder else {
                        log.error("syncOrder of the previous bookmark is nil.")
                        return
                    }
                    previousOrder = previousSyncOrder
                }
            case .down(let toTheBottom):
                // Going down pushes all bookmark up, so the destinatoin bookmark is before the source bookmark.
                previousOrder = destinationBookmarkSyncOrder
                if !toTheBottom {
                    guard let nextSyncOrder = nextOrPreviousBookmark?.syncOrder else {
                        log.error("syncOrder of the next bookmark is nil.")
                        return
                    }
                    nextOrder = nextSyncOrder
                }
            }
            
            guard let updatedSyncOrder = Sync.shared.getBookmarkOrder(previousOrder: previousOrder, nextOrder: nextOrder) else {
                log.error("updated syncOrder from the javascript method was nil")
                return
            }
            
            srcBookmark.syncOrder = updatedSyncOrder
            // Now that we updated the `syncOrder` we have to go through all Bookmarks and update its `order`
            // attributes.
            self.setOrderForAllBookmarksOnGivenLevel(parent: srcBookmark.parentFolder,
                                                     forFavorites: srcBookmark.isFavorite, context: context)
            if !srcBookmark.isFavorite {
                Sync.shared.sendSyncRecords(action: .update, records: [srcBookmark])
            }
            
            if isInteractiveDragReorder && context.hasChanges {
                do {
                    assert(Thread.isMainThread)
                    try context.save()
                } catch {
                    log.error("performTask save error: \(error)")
                }
            }
        }
    }
    
    private enum ReorderMovement {
        case up(toTheTop: Bool)
        case down(toTheBottom: Bool)
    }
    
    private struct FrcReorderData {
        let nextOrPreviousBookmarkId: NSManagedObjectID?
        /// The dragged Bookmark can go in two directions each having two types, so 4 ways in total:
        /// 1. Go all way to the top
        /// 2. Go up(between two Bookmarks)
        /// 3. Go all way to the bottom
        /// 4. Go down(between two Bookmarks)
        let reorderMovement: ReorderMovement
    }
    
    private class func getReorderData(fromFrc frc: NSFetchedResultsController<Bookmark>,
                                      sourceIndexPath src: IndexPath,
                                      destinationIndexPath dest: IndexPath) -> FrcReorderData? {
        
        var data: FrcReorderData?
        
        guard let count = frc.fetchedObjects?.count else {
            log.error("frc.fetchedObject is nil")
            return nil
        }
        
        var nextOrPreviousBookmarkObjectId: NSManagedObjectID?
        var reorderMovement: ReorderMovement?
        
        let isMovingUp = src.row > dest.row
        
        if isMovingUp {
            let bookmarkMovedToTop = dest.row == 0
            if !bookmarkMovedToTop {
                let previousBookmarkIndex = IndexPath(row: dest.row - 1, section: dest.section)
                nextOrPreviousBookmarkObjectId = frc.object(at: previousBookmarkIndex).objectID
            }
            
            reorderMovement = bookmarkMovedToTop ? .up(toTheTop: true) : .up(toTheTop: false)
        } else {
            let bookmarkMovedToBottom = dest.row + 1 >= count
            if !bookmarkMovedToBottom {
                let nextBookmarkIndex = IndexPath(row: dest.row + 1, section: dest.section)
                nextOrPreviousBookmarkObjectId = frc.object(at: nextBookmarkIndex).objectID
            }
            
            reorderMovement = bookmarkMovedToBottom ? .down(toTheBottom: true) : .down(toTheBottom: false)
        }
        
        guard let movement = reorderMovement else { fatalError() }
        
        data = FrcReorderData(nextOrPreviousBookmarkId: nextOrPreviousBookmarkObjectId,
                              reorderMovement: movement)
        
        return data
    }
}

private extension NSManagedObjectContext {
    /// Returns a Bookmark for a given object id.
    /// This operation is thread-safe.
    func bookmark(with id: NSManagedObjectID) -> Bookmark? {
        return self.object(with: id) as? Bookmark
    }
}
