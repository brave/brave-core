// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import UIKit
import Web

// MARK: UICollectionViewDelegate

extension TabTrayController: UICollectionViewDelegate {
  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    guard let tabID = dataSource.itemIdentifier(for: indexPath), let tab = tabManager[tabID] else {
      return
    }
    tabManager.selectTab(tab)

    tabTraySearchController.isActive = false
    dismiss(animated: true)
  }
}

// MARK: UICollectionViewDragDelegate

extension TabTrayController: UICollectionViewDragDelegate {
  func collectionView(
    _ collectionView: UICollectionView,
    itemsForBeginning session: UIDragSession,
    at indexPath: IndexPath
  ) -> [UIDragItem] {
    guard let tabID = dataSource.itemIdentifier(for: indexPath) else { return [] }

    UIImpactFeedbackGenerator(style: .medium).vibrate()

    let dragItem = UIDragItem(itemProvider: NSItemProvider())
    dragItem.localObject = tabID
    return [dragItem]
  }
}

// MARK: UICollectionViewDropDelegate

extension TabTrayController: UICollectionViewDropDelegate {
  func collectionView(
    _ collectionView: UICollectionView,
    performDropWith coordinator: UICollectionViewDropCoordinator
  ) {

    guard let dragItem = coordinator.items.first?.dragItem,
      let tabID = dragItem.localObject as? TabState.ID,
      let tab = tabManager[tabID],
      let destinationIndexPath = coordinator.destinationIndexPath
    else { return }

    _ = coordinator.drop(dragItem, toItemAt: destinationIndexPath)
    tabManager.moveTab(tab, toIndex: destinationIndexPath.item)
    delegate?.tabOrderChanged()
    applySnapshot()
  }

  func collectionView(
    _ collectionView: UICollectionView,
    dropSessionDidUpdate session: UIDropSession,
    withDestinationIndexPath destinationIndexPath: IndexPath?
  ) -> UICollectionViewDropProposal {

    guard let localDragSession = session.localDragSession,
      let item = localDragSession.items.first,
      let tabID = item.localObject as? TabState.ID
    else {
      return .init(operation: .forbidden)
    }

    if dataSource.indexPath(for: tabID) == nil {
      return .init(operation: .cancel)
    }

    return .init(operation: .move, intent: .insertAtDestinationIndexPath)
  }
}
