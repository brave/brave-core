// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Data
import Shared
import UIKit

// MARK: - ContextMenu

extension TopSitesViewController {
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
    case .topSites:
      guard let tile = tiles[safe: indexPath.item] else { return nil }
      return UIContextMenuConfiguration(identifier: indexPath as NSCopying, previewProvider: nil) {
        _ -> UIMenu? in
        let openInNewTab = UIAction(
          title: Strings.openNewTabButtonTitle,
          handler: UIAction.deferredActionHandler { _ in
            self.topSiteAction(
              .opened(
                url: tile.url,
                isFavorite: tile.isFavorite,
                inNewTab: true,
                switchingToPrivateMode: false
              )
            )
          }
        )

        var urlChildren: [UIAction] = [openInNewTab]
        if !self.privateBrowsingManager.isPrivateBrowsing {
          let openInNewPrivateTab = UIAction(
            title: Strings.openNewPrivateTabButtonTitle,
            handler: UIAction.deferredActionHandler { _ in
              self.topSiteAction(
                .opened(
                  url: tile.url,
                  isFavorite: tile.isFavorite,
                  inNewTab: true,
                  switchingToPrivateMode: true
                )
              )
            }
          )
          urlChildren.append(openInNewPrivateTab)
        }

        let modeChildren: [UIAction]
        switch tile.id {
        case .favorite(let objectID):
          modeChildren = [
            UIAction(
              title: Strings.editFavorite,
              handler: UIAction.deferredActionHandler { _ in
                guard let favorite = Favorite.get(with: objectID) else { return }
                self.topSiteAction(.edited(favorite: favorite))
              }
            ),
            UIAction(
              title: Strings.removeFavorite,
              attributes: .destructive,
              handler: UIAction.deferredActionHandler { _ in
                Favorite.get(with: objectID)?.delete()
              }
            ),
          ]
        case .mostVisited:
          modeChildren = [
            UIAction(
              title: Strings.excludeMostVisitedSite,
              attributes: .destructive,
              handler: UIAction.deferredActionHandler { _ in
                self.topSiteAction(.excluded(tile: tile))
              }
            )
          ]
        }

        return UIMenu(
          title: tile.title ?? tile.url.absoluteString,
          identifier: nil,
          children: [
            UIMenu(title: "", options: .displayInline, children: urlChildren),
            UIMenu(title: "", options: .displayInline, children: modeChildren),
          ]
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
    guard let cell = collectionView.cellForItem(at: indexPath) as? TopSitesCollectionViewCell
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
    guard let cell = collectionView.cellForItem(at: indexPath) as? TopSitesCollectionViewCell
    else {
      return nil
    }
    return UITargetedPreview(view: cell.imageContainer)
  }
}
