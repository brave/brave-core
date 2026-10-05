// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Data
import Shared
import UIKit

// MARK: - UICollectionViewDragDelegate & UICollectionViewDropDelegate

extension TopSitesViewController: UICollectionViewDragDelegate, UICollectionViewDropDelegate {
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
    case .topSites:
      // Only favorites can be reordered, and only when there is more than one.
      guard tileSource.isReorderingEnabled,
        let tile = tiles[safe: indexPath.item],
        case .favorite(let objectID) = tile.id,
        let favorite = Favorite.get(with: objectID)
      else {
        return []
      }
      let itemProvider = NSItemProvider(object: "\(indexPath)" as NSString)
      let dragItem = UIDragItem(itemProvider: itemProvider)
      dragItem.previewProvider = { () -> UIDragPreview? in
        guard let cell = collectionView.cellForItem(at: indexPath) as? TopSitesCollectionViewCell
        else {
          return nil
        }
        return UIDragPreview(view: cell.imageContainer)
      }
      dragItem.localObject = favorite
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
      guard tileSource.isReorderingEnabled,
        let item = coordinator.items.first
      else { return }
      _ = coordinator.drop(item.dragItem, toItemAt: destinationIndexPath)
      Favorite.reorder(
        sourceIndexPath: sourceIndexPath,
        destinationIndexPath: destinationIndexPath,
        isInteractiveDragReorder: true
      )
      // The reorder writes synchronously on the view context, so the tiles are already current.
      // Applying here rather than waiting for the tile source's async notification keeps the
      // snapshot in step with the drop animation.
      updateUIWithSnapshot(animated: true)
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
    guard tileSource.isReorderingEnabled,
      let destinationIndexPath,
      availableSections[safe: destinationIndexPath.section] == .topSites
    else {
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
    if let cell = collectionView.cellForItem(at: indexPath) as? TopSitesCollectionViewCell {
      params.visiblePath = UIBezierPath(roundedRect: cell.imageContainer.frame, cornerRadius: 8)
    }
    return params
  }

  func collectionView(
    _ collectionView: UICollectionView,
    dropPreviewParametersForItemAt indexPath: IndexPath
  ) -> UIDragPreviewParameters? {
    let params = UIDragPreviewParameters()
    params.backgroundColor = .clear
    if let cell = collectionView.cellForItem(at: indexPath) as? TopSitesCollectionViewCell {
      params.visiblePath = UIBezierPath(roundedRect: cell.imageContainer.frame, cornerRadius: 8)
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
