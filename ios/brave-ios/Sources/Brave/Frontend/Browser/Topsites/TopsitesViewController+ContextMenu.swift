// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Shared
import UIKit

// MARK: - ContextMenu

extension TopsitesViewController {
  func collectionView(
    _ collectionView: UICollectionView,
    contextMenuConfigurationForItemsAt indexPaths: [IndexPath],
    point: CGPoint
  ) -> UIContextMenuConfiguration? {
    guard let indexPath = indexPaths.first,
      let section = availableSections[safe: indexPath.section]
    else {
      assertionFailure("Invalid Section")
      return nil
    }

    switch section {
    case .topsites:
      guard let topsiteViewModel = tileSource[indexPath.item] else { return nil }
      return UIContextMenuConfiguration(identifier: indexPath as NSCopying, previewProvider: nil) {
        _ -> UIMenu? in
        let openInNewTab = UIAction(
          title: Strings.openNewTabButtonTitle,
          handler: UIAction.deferredActionHandler { _ in
            self.topSiteAction(
              .opened(
                url: favorite.url?.asURL,
                isFavorite: true,
                inNewTab: true,
                switchingToPrivateMode: false
              )
            )
          }
        )
        var children: [UIMenuElement] = []
        if case .favorite(let favorite) = topsiteViewModel.source {
          let edit = UIAction(
            title: Strings.editFavorite,
            handler: UIAction.deferredActionHandler { _ in
              self.topsiteAction(.edited(favorite: favorite))
            }
          )
          let delete = UIAction(
            title: Strings.removeFavorite,
            attributes: .destructive,
            handler: UIAction.deferredActionHandler { _ in
              favorite.delete()
            }
          )

          let favMenu = UIMenu(title: "", options: .displayInline, children: [edit, delete])
          children.append(favMenu)
        }

        var urlChildren: [UIAction] = [openInNewTab]
        if !self.privateBrowsingManager.isPrivateBrowsing {
          let openInNewPrivateTab = UIAction(
            title: Strings.openNewPrivateTabButtonTitle,
            handler: UIAction.deferredActionHandler { _ in
              self.topSiteAction(
                .opened(
                  url: favorite.url?.asURL,
                  isFavorite: true,
                  inNewTab: true,
                  switchingToPrivateMode: true
                )
              )
            }
          )
          urlChildren.append(openInNewPrivateTab)
        }

        let urlMenu = UIMenu(title: "", options: .displayInline, children: urlChildren)
        children.append(urlMenu)
        return UIMenu(
          title: topsiteViewModel.title ?? topsiteViewModel.url?.absoluteString ?? "",
          identifier: nil,
          children: children
        )
      }
    case .recentSearches, .recentSearchesOptIn:
      break
    }
    return nil
  }

  func collectionView(
    _ collectionView: UICollectionView,
    contextMenuConfiguration configuration: UIContextMenuConfiguration,
    highlightPreviewForItemAt indexPath: IndexPath
  ) -> UITargetedPreview? {
    guard let cell = collectionView.cellForItem(at: indexPath) as? TopsitesCollectionViewCell
    else {
      return nil
    }
    return UITargetedPreview(view: cell.imageContainer)
  }

  func collectionView(
    _ collectionView: UICollectionView,
    contextMenuConfiguration configuration: UIContextMenuConfiguration,
    dismissalPreviewForItemAt indexPath: IndexPath
  ) -> UITargetedPreview? {
    guard let cell = collectionView.cellForItem(at: indexPath) as? TopsitesCollectionViewCell
    else {
      return nil
    }
    return UITargetedPreview(view: cell.imageContainer)
  }
}
