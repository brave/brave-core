// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import CoreData
import Data
import Foundation
import Preferences
import Shared
import UIKit
import os.log

enum BookmarksAction {
  case opened(inNewTab: Bool = false, switchingToPrivateMode: Bool = false)
  case edited
}

class FavoritesSectionProvider: NSObject, NTPObservableSectionProvider {
  var sectionDidChange: (() -> Void)?
  var action: (Favorite, BookmarksAction) -> Void
  var legacyLongPressAction: (UIAlertController) -> Void

  private let isPrivateBrowsing: Bool

  var hasMoreThanOneFavouriteItems: Bool {
    frc.fetchedObjects?.count ?? 0 > 0
  }

  private var frc: NSFetchedResultsController<Favorite>

  init(
    action: @escaping (Favorite, BookmarksAction) -> Void,
    legacyLongPressAction: @escaping (UIAlertController) -> Void,
    isPrivateBrowsing: Bool
  ) {
    self.action = action
    self.legacyLongPressAction = legacyLongPressAction
    self.isPrivateBrowsing = isPrivateBrowsing

    frc = Favorite.frc()
    super.init()
    frc.fetchRequest.fetchLimit = 20
    frc.delegate = self

    do {
      try frc.performFetch()
    } catch {
      Logger.module.error("Favorites fetch error")
    }
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

  var numberOfFavorites: Int {
    frc.fetchedObjects?.count ?? 0
  }

  /// The actual number of favorites that will be displayed in a single row
  /// given the available width, which is the lesser of the number of fetched
  /// favorites and the maximum number of items that fit in the row.
  func displayedItemCount(in collectionView: UICollectionView, section: Int) -> Int {
    guard Preferences.NewTabPage.showNewTabFavourites.value else { return 0 }
    return min(
      numberOfFavorites,
      Self.numberOfItems(
        in: collectionView,
        availableWidth: fittingSizeForCollectionView(collectionView, section: section).width
      )
    )
  }

  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    guard let bookmark = frc.fetchedObjects?[safe: indexPath.item] else {
      return
    }
    action(bookmark, .opened())
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

    guard let cell = cell as? FavoritesCell else {
      return
    }

    let fav = frc.object(at: IndexPath(item: indexPath.item, section: 0))
    cell.title = fav.displayTitle ?? fav.url

    // Reset Fav-icon loading and image-view to default
    cell.imageView.cancelLoading()

    if let url = fav.url?.asURL {
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
    contextMenuConfigurationForItemAt indexPath: IndexPath,
    point: CGPoint
  ) -> UIContextMenuConfiguration? {
    guard let favourite = frc.fetchedObjects?[indexPath.item] else { return nil }
    return UIContextMenuConfiguration(identifier: indexPath as NSCopying, previewProvider: nil) {
      _ -> UIMenu? in
      let openInNewTab = UIAction(
        title: Strings.openNewTabButtonTitle,
        handler: UIAction.deferredActionHandler { _ in
          self.action(favourite, .opened(inNewTab: true, switchingToPrivateMode: false))
        }
      )
      let edit = UIAction(
        title: Strings.editFavorite,
        handler: UIAction.deferredActionHandler { _ in
          self.action(favourite, .edited)
        }
      )
      let delete = UIAction(
        title: Strings.removeFavorite,
        attributes: .destructive,
        handler: UIAction.deferredActionHandler { _ in
          favourite.delete()
        }
      )

      var urlChildren: [UIAction] = [openInNewTab]
      if !self.isPrivateBrowsing {
        let openInNewPrivateTab = UIAction(
          title: Strings.openNewPrivateTabButtonTitle,
          handler: UIAction.deferredActionHandler { _ in
            self.action(favourite, .opened(inNewTab: true, switchingToPrivateMode: true))
          }
        )
        urlChildren.append(openInNewPrivateTab)
      }

      let urlMenu = UIMenu(title: "", options: .displayInline, children: urlChildren)
      let favMenu = UIMenu(title: "", options: .displayInline, children: [edit, delete])
      return UIMenu(
        title: favourite.title ?? favourite.url ?? "",
        identifier: nil,
        children: [urlMenu, favMenu]
      )
    }
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
    previewForDismissingContextMenuWithConfiguration configuration: UIContextMenuConfiguration
  ) -> UITargetedPreview? {
    guard let indexPath = configuration.identifier as? IndexPath,
      let cell = collectionView.cellForItem(at: indexPath) as? FavoritesCell
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

extension FavoritesSectionProvider: NSFetchedResultsControllerDelegate {
  func controllerDidChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
    try? frc.performFetch()
    DispatchQueue.main.async {
      self.sectionDidChange?()
    }
  }
}
