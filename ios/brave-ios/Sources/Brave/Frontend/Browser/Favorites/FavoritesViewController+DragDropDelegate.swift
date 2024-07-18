// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Data
import Shared
import UIKit

// MARK: - UICollectionViewDragDelegate & UICollectionViewDropDelegate

extension FavoritesViewController: UICollectionViewDragDelegate, UICollectionViewDropDelegate {
  func collectionView(
    _ collectionView: UICollectionView,
    itemsForBeginning session: UIDragSession,
    at indexPath: IndexPath
  ) -> [UIDragItem] {
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
        guard let cell = collectionView.cellForItem(at: indexPath) as? FavoritesCell else {
          return nil
        }
        return UIDragPreview(view: cell.imageView)
      }
      dragItem.localObject = bookmark
      return [dragItem]
    case .recentSearches, .recentSearchesOptIn:
      break
    }
    return []
  }

  func collectionView(
    _ collectionView: UICollectionView,
    performDropWith coordinator: UICollectionViewDropCoordinator
  ) {
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

  func collectionView(
    _ collectionView: UICollectionView,
    dropSessionDidUpdate session: UIDropSession,
    withDestinationIndexPath destinationIndexPath: IndexPath?
  ) -> UICollectionViewDropProposal {
    if favoritesFRC.fetchedObjects?.count == 1 {
      return .init(operation: .cancel)
    }
    return .init(operation: .move, intent: .insertAtDestinationIndexPath)
  }

  func collectionView(
    _ collectionView: UICollectionView,
    dragPreviewParametersForItemAt indexPath: IndexPath
  ) -> UIDragPreviewParameters? {
    let params = UIDragPreviewParameters()
    params.backgroundColor = .clear
    if let cell = collectionView.cellForItem(at: indexPath) as? FavoritesCell {
      params.visiblePath = UIBezierPath(roundedRect: cell.imageView.frame, cornerRadius: 8)
    }
    return params
  }

  func collectionView(
    _ collectionView: UICollectionView,
    dropPreviewParametersForItemAt indexPath: IndexPath
  ) -> UIDragPreviewParameters? {
    let params = UIDragPreviewParameters()
    params.backgroundColor = .clear
    if let cell = collectionView.cellForItem(at: indexPath) as? FavoritesCell {
      params.visiblePath = UIBezierPath(roundedRect: cell.imageView.frame, cornerRadius: 8)
    }
    return params
  }

  func collectionView(
    _ collectionView: UICollectionView,
    dragSessionIsRestrictedToDraggingApplication session: UIDragSession
  ) -> Bool {
    return true
  }
}
