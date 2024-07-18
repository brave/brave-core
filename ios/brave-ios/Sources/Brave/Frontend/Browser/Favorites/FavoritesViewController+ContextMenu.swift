// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Shared
import UIKit

// MARK: - ContextMenu

extension FavoritesViewController {
  func collectionView(
    _ collectionView: UICollectionView,
    contextMenuConfigurationForItemAt indexPath: IndexPath,
    point: CGPoint
  ) -> UIContextMenuConfiguration? {
    guard let section = availableSections[safe: indexPath.section] else {
      assertionFailure("Invalid Section")
      return nil
    }

    switch section {
    case .pasteboard:
      break
    case .favorites:
      guard let bookmark = favoritesFRC.fetchedObjects?[indexPath.item] else { return nil }
      return UIContextMenuConfiguration(identifier: indexPath as NSCopying, previewProvider: nil) {
        _ -> UIMenu? in
        let openInNewTab = UIAction(
          title: Strings.openNewTabButtonTitle,
          handler: UIAction.deferredActionHandler { _ in
            self.bookmarkAction(bookmark, .opened(inNewTab: true, switchingToPrivateMode: false))
          }
        )
        let edit = UIAction(
          title: Strings.editFavorite,
          handler: UIAction.deferredActionHandler { _ in
            self.bookmarkAction(bookmark, .edited)
          }
        )
        let delete = UIAction(
          title: Strings.removeFavorite,
          attributes: .destructive,
          handler: UIAction.deferredActionHandler { _ in
            bookmark.delete()
          }
        )

        var urlChildren: [UIAction] = [openInNewTab]
        if !self.privateBrowsingManager.isPrivateBrowsing {
          let openInNewPrivateTab = UIAction(
            title: Strings.openNewPrivateTabButtonTitle,
            handler: UIAction.deferredActionHandler { _ in
              self.bookmarkAction(bookmark, .opened(inNewTab: true, switchingToPrivateMode: true))
            }
          )
          urlChildren.append(openInNewPrivateTab)
        }

        let urlMenu = UIMenu(title: "", options: .displayInline, children: urlChildren)
        let favMenu = UIMenu(title: "", options: .displayInline, children: [edit, delete])
        return UIMenu(
          title: bookmark.title ?? bookmark.url ?? "",
          identifier: nil,
          children: [urlMenu, favMenu]
        )
      }
    case .recentSearches, .recentSearchesOptIn:
      break
    }
    return nil
  }

  func collectionView(
    _ collectionView: UICollectionView,
    previewForHighlightingContextMenuWithConfiguration configuration: UIContextMenuConfiguration
  ) -> UITargetedPreview? {
    guard let indexPath = configuration.identifier as? IndexPath,
      let cell = collectionView.cellForItem(at: indexPath) as? FavoritesCell
    else {
      return nil
    }
    return UITargetedPreview(view: cell.imageView)
  }

  func collectionView(
    _ collectionView: UICollectionView,
    previewForDismissingContextMenuWithConfiguration configuration: UIContextMenuConfiguration
  ) -> UITargetedPreview? {
    guard let indexPath = configuration.identifier as? IndexPath,
      let cell = collectionView.cellForItem(at: indexPath) as? FavoritesCell
    else {
      return nil
    }
    return UITargetedPreview(view: cell.imageView)
  }
}
