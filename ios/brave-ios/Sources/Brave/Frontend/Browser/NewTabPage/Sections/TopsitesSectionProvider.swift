// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import Data
import Foundation
import Shared
import UIKit

enum TopsiteAction {
  case opened(
    topsiteViewModel: TopsiteViewModel,
    inNewTab: Bool = false,
    switchingToPrivateMode: Bool = false
  )
  case edited(favorite: Favorite)
  case excluded(onConfirm: () -> Void)
}

struct TopsiteViewModel {
  enum Source {
    case favorite(Favorite)
    case mostVisited(NTPTile)
  }

  let source: Source

  var url: URL? {
    switch source {
    case .favorite(let favorite): return favorite.url?.asURL
    case .mostVisited(let tile): return tile.url as URL
    }
  }

  var title: String? {
    switch source {
    case .favorite(let favorite): return favorite.displayTitle ?? favorite.url
    case .mostVisited(let tile): return tile.title
    }
  }

  var isFavorite: Bool {
    if case .favorite = source { return true }
    return false
  }
}

class TopsitesSectionProvider: NSObject, NTPObservableSectionProvider {
  var sectionDidChange: (() -> Void)?
  var action: (TopsiteAction) -> Void
  var legacyLongPressAction: (UIAlertController) -> Void

  private let isPrivateBrowsing: Bool
  private let tileSource: TopsitesTileSource

  var isReorderingEnabled: Bool {
    tileSource.isReorderingEnabled
  }

  init(
    action: @escaping (TopsiteAction) -> Void,
    legacyLongPressAction: @escaping (UIAlertController) -> Void,
    isPrivateBrowsing: Bool,
    tileSource: TopsitesTileSource
  ) {
    self.action = action
    self.legacyLongPressAction = legacyLongPressAction
    self.isPrivateBrowsing = isPrivateBrowsing
    self.tileSource = tileSource

    super.init()

    tileSource.addObserver(self)
  }

  static var defaultIconSize = CGSize(width: 64, height: FavoritesCell.height(forWidth: 64))

  /// The maximum width of the favorites content, matching the stats section.
  static let maxWidth: CGFloat = 640

  /// The number of times that each row contains
  static func numberOfItems(in collectionView: UICollectionView, availableWidth: CGFloat) -> Int {
    let defaultWidth: CGFloat = defaultIconSize.width
    return Int(floor(availableWidth / defaultWidth))
  }

  func registerCells(to collectionView: UICollectionView) {
    collectionView.register(
      FavoritesCell.self,
      forCellWithReuseIdentifier: FavoritesCell.identifier
    )
  }

  /// The actual number of favorites that will be displayed in a single row
  /// given the available width, which is the lesser of the number of fetched
  /// favorites and the maximum number of items that fit in the row.
  func displayedItemCount(in collectionView: UICollectionView, section: Int) -> Int {
    return min(
      tileSource.count,
      Self.numberOfItems(
        in: collectionView,
        availableWidth: fittingSizeForCollectionView(collectionView, section: section).width
      )
    )
  }

  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    guard let item = tileSource[indexPath.item] else { return }
    action(.opened(topsiteViewModel: item))
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
      withReuseIdentifier: FavoritesCell.identifier,
      for: indexPath
    )
  }

  func collectionView(
    _ collectionView: UICollectionView,
    willDisplay cell: UICollectionViewCell,
    forItemAt indexPath: IndexPath
  ) {

    guard let cell = cell as? FavoritesCell,
      let item = tileSource[indexPath.item]
    else {
      return
    }
    cell.title = item.title
    // Reset Fav-icon loading and image-view to default
    cell.imageView.cancelLoading()
    if let url = item.url {
      cell.imageView.loadFavicon(siteURL: url, isPrivateBrowsing: isPrivateBrowsing)
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
        height: FavoritesCell.height(forWidth: floor(width / 4.0))
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
      let item = tileSource[indexPath.item]
    else { return nil }

    return UIContextMenuConfiguration(identifier: indexPath as NSCopying, previewProvider: nil) {
      _ -> UIMenu? in
      let openInNewTab = UIAction(
        title: Strings.openNewTabButtonTitle,
        handler: UIAction.deferredActionHandler { _ in
          self.action(
            .opened(
              topsiteViewModel: item,
              inNewTab: true,
              switchingToPrivateMode: false
            )
          )
        }
      )
      var urlChildren = [openInNewTab]
      if !self.isPrivateBrowsing {
        urlChildren.append(
          UIAction(
            title: Strings.openNewPrivateTabButtonTitle,
            handler: UIAction.deferredActionHandler { _ in
              self.action(
                .opened(
                  topsiteViewModel: item,
                  inNewTab: true,
                  switchingToPrivateMode: true
                )
              )
            }
          )
        )
      }

      let modeChildren: [UIAction]
      switch item.source {
      case .favorite(let favorite):
        modeChildren = [
          UIAction(
            title: Strings.editFavorite,
            handler: UIAction.deferredActionHandler { _ in
              self.action(.edited(favorite: favorite))
            }
          ),
          UIAction(
            title: Strings.removeFavorite,
            attributes: .destructive,
            handler: UIAction.deferredActionHandler { _ in
              favorite.delete()
            }
          ),
        ]
      case .mostVisited(let ntpTile):
        modeChildren = [
          UIAction(
            title: Strings.excludeMostVisitedSite,
            attributes: .destructive,
            handler: UIAction.deferredActionHandler { _ in
              self.action(
                .excluded(
                  onConfirm: { [weak self] in
                    self?.tileSource.exclude(ntpTile)
                  })
              )
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
    guard let cell = collectionView.cellForItem(at: indexPath) as? FavoritesCell
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
    guard let cell = collectionView.cellForItem(at: indexPath) as? FavoritesCell
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

extension TopsitesSectionProvider: TopsitesTileSourceObserver {
  func topsitesTileSourceDidChangeTiles(_ source: TopsitesTileSource) {
    sectionDidChange?()
  }

  // Favicons are loaded by the cell itself, so favicon updates are ignored here.
}
