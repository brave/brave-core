// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import Data
import Foundation
import Observation
import Shared
import UIKit

enum TopSiteAction {
  case opened(
    url: URL?,
    isFavorite: Bool = false,
    inNewTab: Bool = false,
    switchingToPrivateMode: Bool = false
  )
  case edited(favorite: Favorite)
  case excluded(tile: TopSiteTile)
}

class TopSitesSectionProvider: NSObject, NTPObservableSectionProvider {
  var sectionDidChange: (() -> Void)?
  var action: (TopSiteAction) -> Void

  private let isPrivateBrowsing: Bool
  private let tileSource: TopSitesTileSource
  /// The tiles the collection view is currently showing. Kept separate from the source so that the
  /// item count and the items themselves can't disagree while a reload is pending (drag to order)
  private var tiles: [TopSiteTile] = []

  var isReorderingEnabled: Bool {
    tileSource.isReorderingEnabled
  }

  init(
    action: @escaping (TopSiteAction) -> Void,
    isPrivateBrowsing: Bool,
    tileSource: TopSitesTileSource
  ) {
    self.action = action
    self.isPrivateBrowsing = isPrivateBrowsing
    self.tileSource = tileSource

    super.init()

    updateTiles()
  }

  /// Snapshots the source's tiles and re-arms tracking for the next change.
  private func updateTiles() {
    tiles = withObservationTracking {
      tileSource.tiles
    } onChange: { [weak self] in
      DispatchQueue.main.async {
        self?.updateTiles()
        self?.sectionDidChange?()
      }
    }
  }

  static var defaultIconSize = CGSize(width: 64, height: TopSitesCell.height(forWidth: 64))

  /// The maximum width of the favorites content, matching the stats section.
  static let maxWidth: CGFloat = 640

  /// The number of times that each row contains
  static func numberOfItems(in collectionView: UICollectionView, availableWidth: CGFloat) -> Int {
    let defaultWidth: CGFloat = defaultIconSize.width
    return Int(floor(availableWidth / defaultWidth))
  }

  func registerCells(to collectionView: UICollectionView) {
    collectionView.register(
      TopSitesCell.self,
      forCellWithReuseIdentifier: TopSitesCell.identifier
    )
  }

  /// The actual number of favorites that will be displayed in a single row
  /// given the available width, which is the lesser of the number of fetched
  /// favorites and the maximum number of items that fit in the row.
  func displayedItemCount(in collectionView: UICollectionView, section: Int) -> Int {
    return min(
      tiles.count,
      Self.numberOfItems(
        in: collectionView,
        availableWidth: fittingSizeForCollectionView(collectionView, section: section).width
      )
    )
  }

  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    guard let item = tiles[safe: indexPath.item] else { return }
    action(.opened(url: item.url, isFavorite: item.isFavorite))
  }

  func collectionView(
    _ collectionView: UICollectionView,
    numberOfItemsInSection section: Int
  ) -> Int {
    return displayedItemCount(in: collectionView, section: section)
  }

  func collectionView(
    _ collectionView: UICollectionView,
    cellForItemAt indexPath: IndexPath
  ) -> UICollectionViewCell {
    return collectionView.dequeueReusableCell(
      withReuseIdentifier: TopSitesCell.identifier,
      for: indexPath
    )
  }

  func collectionView(
    _ collectionView: UICollectionView,
    willDisplay cell: UICollectionViewCell,
    forItemAt indexPath: IndexPath
  ) {

    guard let cell = cell as? TopSitesCell,
      let item = tiles[safe: indexPath.item]
    else {
      return
    }
    cell.title = item.title
    // Any change to the tiles reloads the whole collection view, so most cells come back showing
    // the favicon they already had. Reloading those would blank the icon until the load finishes,
    // which reads as every tile flashing away when only one of them changed.
    if cell.loadedSiteURL != item.url {
      // Reset Fav-icon loading and image-view to default
      cell.imageView.cancelLoading()
      cell.imageView.loadFavicon(siteURL: item.url, isPrivateBrowsing: isPrivateBrowsing)
      cell.loadedSiteURL = item.url
    }
    cell.accessibilityLabel = cell.title
  }

  private func itemSize(collectionView: UICollectionView, section: Int) -> CGSize {
    let width = fittingSizeForCollectionView(collectionView, section: section).width
    var size = Self.defaultIconSize

    let minimumNumberOfColumns = Self.numberOfItems(in: collectionView, availableWidth: width)
    let minWidth = floor(width / CGFloat(minimumNumberOfColumns))
    if minWidth < size.width {
      // If the default icon size is too large, make it slightly smaller
      // to fit at least 4 icons
      size = CGSize(
        width: floor(width / 4.0),
        height: TopSitesCell.height(forWidth: floor(width / 4.0))
      )
    }
    return size
  }

  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    sizeForItemAt indexPath: IndexPath
  ) -> CGSize {
    return itemSize(collectionView: collectionView, section: indexPath.section)
  }

  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    insetForSectionAt section: Int
  ) -> UIEdgeInsets {
    let insets = horizontalInsets(
      for: collectionView,
      maxWidth: Self.maxWidth,
      minimumInset: 16
    )
    return UIEdgeInsets(top: 8, left: insets.left, bottom: 8, right: insets.right)
  }

  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    minimumInteritemSpacingForSectionAt section: Int
  ) -> CGFloat {
    let width = fittingSizeForCollectionView(collectionView, section: section).width
    let size = itemSize(collectionView: collectionView, section: section)
    let numberOfItems = Self.numberOfItems(in: collectionView, availableWidth: width)

    return floor((width - (size.width * CGFloat(numberOfItems))) / (CGFloat(numberOfItems) - 1))
  }

  func collectionView(
    _ collectionView: UICollectionView,
    contextMenuConfigurationForItemsAt indexPaths: [IndexPath],
    point: CGPoint
  ) -> UIContextMenuConfiguration? {
    guard let indexPath = indexPaths.first,
      let item = tiles[safe: indexPath.item]
    else { return nil }

    let isPrivate = isPrivateBrowsing
    return UIContextMenuConfiguration(identifier: indexPath as NSCopying, previewProvider: nil) {
      [unowned self] _ -> UIMenu? in
      let openInNewTab = UIAction(
        title: Strings.openNewTabButtonTitle,
        handler: UIAction.deferredActionHandler { _ in
          self.action(
            .opened(
              url: item.url,
              isFavorite: item.isFavorite,
              inNewTab: true,
              switchingToPrivateMode: false
            )
          )
        }
      )
      var urlChildren = [openInNewTab]
      if !isPrivate {
        urlChildren.append(
          UIAction(
            title: Strings.openNewPrivateTabButtonTitle,
            handler: UIAction.deferredActionHandler { _ in
              self.action(
                .opened(
                  url: item.url,
                  isFavorite: item.isFavorite,
                  inNewTab: true,
                  switchingToPrivateMode: true
                )
              )
            }
          )
        )
      }

      let modeChildren: [UIAction]
      switch item.id {
      case .favorite(let objectID):
        modeChildren = [
          UIAction(
            title: Strings.editFavorite,
            handler: UIAction.deferredActionHandler { _ in
              guard let favorite = Favorite.get(with: objectID) else { return }
              self.action(.edited(favorite: favorite))
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
              self.action(.excluded(tile: item))
            }
          )
        ]
      }

      return UIMenu(
        title: item.title ?? "",
        children: [
          UIMenu(title: "", options: .displayInline, children: urlChildren),
          UIMenu(title: "", options: .displayInline, children: modeChildren),
        ]
      )
    }
  }

  func collectionView(
    _ collectionView: UICollectionView,
    contextMenuConfiguration configuration: UIContextMenuConfiguration,
    highlightPreviewForItemAt indexPath: IndexPath
  ) -> UITargetedPreview? {
    guard let cell = collectionView.cellForItem(at: indexPath) as? TopSitesCell
    else {
      return nil
    }
    let preview = UITargetedPreview(view: cell.imageContainerView)
    preview.parameters.backgroundColor = .clear
    preview.parameters.visiblePath = UIBezierPath(
      roundedRect: cell.imageContainerView.bounds,
      cornerRadius: 16
    )
    return preview
  }

  func collectionView(
    _ collectionView: UICollectionView,
    contextMenuConfiguration configuration: UIContextMenuConfiguration,
    dismissalPreviewForItemAt indexPath: IndexPath
  ) -> UITargetedPreview? {
    guard let cell = collectionView.cellForItem(at: indexPath) as? TopSitesCell
    else {
      return nil
    }
    let preview = UITargetedPreview(view: cell.imageContainerView)
    preview.parameters.backgroundColor = .clear
    preview.parameters.visiblePath = UIBezierPath(
      roundedRect: cell.imageContainerView.bounds,
      cornerRadius: 16
    )
    return preview
  }
}
