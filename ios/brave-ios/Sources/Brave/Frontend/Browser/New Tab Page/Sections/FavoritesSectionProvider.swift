// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI
import Preferences
import Data
import CoreData
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
    frc.fetchRequest.fetchLimit = 10
    frc.delegate = self

    do {
      try frc.performFetch()
    } catch {
      Logger.module.error("Favorites fetch error")
    }
  }

  static var defaultIconSize = CGSize(width: 82, height: FavoriteCell.height(forWidth: 82))
  static var largerIconSize = CGSize(width: 100, height: FavoriteCell.height(forWidth: 100))

  /// The number of times that each row contains
  static func numberOfItems(in collectionView: UICollectionView, availableWidth: CGFloat) -> Int {
    // Two considerations:
    // 1. icon size minimum
    // 2. trait collection
    // 3. orientation ("is landscape")
    let icons = (min: 4, max: 6)
    let defaultWidth: CGFloat = defaultIconSize.width
    let fittingNumber: Int

    if collectionView.traitCollection.horizontalSizeClass == .regular {
      if collectionView.frame.width > collectionView.frame.height {
        fittingNumber = Int(floor(availableWidth / defaultWidth))
      } else {
        fittingNumber = Int(floor(availableWidth / largerIconSize.width))
      }
    } else {
      fittingNumber = Int(floor(availableWidth / defaultWidth))
    }

    return max(icons.min, min(icons.max, fittingNumber))
  }

  func registerCells(to collectionView: UICollectionView) {
    collectionView.register(FavoriteCell.self, forCellWithReuseIdentifier: FavoriteCell.identifier)
  }

  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    guard let bookmark = frc.fetchedObjects?[safe: indexPath.item] else {
      return
    }
    action(bookmark, .opened())
  }

  func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
    let fetchedCount = frc.fetchedObjects?.count ?? 0
    let numberOfItems = min(fetchedCount, Self.numberOfItems(
      in: collectionView,
      availableWidth: fittingSizeForCollectionView(collectionView, section: section).width))
    return Preferences.NewTabPage.showNewTabFavourites.value ? numberOfItems : 0
  }

  func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    return collectionView.dequeueReusableCell(withReuseIdentifier: FavoriteCell.identifier, for: indexPath)
  }

  func collectionView(_ collectionView: UICollectionView, willDisplay cell: UICollectionViewCell, forItemAt indexPath: IndexPath) {

    guard let cell = cell as? FavoriteCell else {
      return
    }

    let fav = frc.object(at: IndexPath(item: indexPath.item, section: 0))
    cell.textLabel.textColor = .white
    cell.textLabel.text = fav.displayTitle ?? fav.url

    // Reset Fav-icon loading and image-view to default
    cell.imageView.cancelLoading()

    if let url = fav.url?.asURL {
      cell.imageView.loadFavicon(siteURL: url, isPrivateBrowsing: isPrivateBrowsing)
    }
    cell.accessibilityLabel = cell.textLabel.text
  }

  private func itemSize(collectionView: UICollectionView, section: Int) -> CGSize {
    let width = fittingSizeForCollectionView(collectionView, section: section).width
    var size = Self.defaultIconSize

    let minimumNumberOfColumns = Self.numberOfItems(in: collectionView, availableWidth: width)
    let minWidth = floor(width / CGFloat(minimumNumberOfColumns))
    if minWidth < size.width {
      // If the default icon size is too large, make it slightly smaller
      // to fit at least 4 icons
      size = CGSize(width: floor(width / 4.0), height: FavoriteCell.height(forWidth: floor(width / 4.0)))
    } else if collectionView.traitCollection.horizontalSizeClass == .regular {
      // If we're on regular horizontal size class and the computed size
      // of the icon is larger than `largerIconSize`, use `largerIconSize`
      if width / CGFloat(minimumNumberOfColumns) > Self.largerIconSize.width {
        size = Self.largerIconSize
      }
    }
    return size
  }

  func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, sizeForItemAt indexPath: IndexPath) -> CGSize {
    return itemSize(collectionView: collectionView, section: indexPath.section)
  }

  func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, insetForSectionAt section: Int) -> UIEdgeInsets {
    let isLandscape = collectionView.frame.width > collectionView.frame.height
    // Adjust the left-side padding a bit for portrait iPad
    let inset = isLandscape ? 12 : collectionView.readableContentGuide.layoutFrame.origin.x
    return UIEdgeInsets(top: 6, left: inset, bottom: 6, right: inset)
  }

  func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, minimumInteritemSpacingForSectionAt section: Int) -> CGFloat {
    let width = fittingSizeForCollectionView(collectionView, section: section).width
    let size = itemSize(collectionView: collectionView, section: section)
    let numberOfItems = Self.numberOfItems(in: collectionView, availableWidth: width)

    return floor((width - (size.width * CGFloat(numberOfItems))) / (CGFloat(numberOfItems) - 1))
  }

  func collectionView(_ collectionView: UICollectionView, contextMenuConfigurationForItemAt indexPath: IndexPath, point: CGPoint) -> UIContextMenuConfiguration? {
    guard let favourite = frc.fetchedObjects?[indexPath.item] else { return nil }
    return UIContextMenuConfiguration(identifier: indexPath as NSCopying, previewProvider: nil) { _ -> UIMenu? in
      let openInNewTab = UIAction(
        title: Strings.openNewTabButtonTitle,
        handler: UIAction.deferredActionHandler { _ in
          self.action(favourite, .opened(inNewTab: true, switchingToPrivateMode: false))
        })
      let edit = UIAction(
        title: Strings.editFavorite,
        handler: UIAction.deferredActionHandler { _ in
          self.action(favourite, .edited)
        })
      let delete = UIAction(
        title: Strings.removeFavorite, attributes: .destructive,
        handler: UIAction.deferredActionHandler { _ in
          favourite.delete()
        })

      var urlChildren: [UIAction] = [openInNewTab]
      if !self.isPrivateBrowsing {
        let openInNewPrivateTab = UIAction(
          title: Strings.openNewPrivateTabButtonTitle,
          handler: UIAction.deferredActionHandler { _ in
            self.action(favourite, .opened(inNewTab: true, switchingToPrivateMode: true))
          })
        urlChildren.append(openInNewPrivateTab)
      }

      let urlMenu = UIMenu(title: "", options: .displayInline, children: urlChildren)
      let favMenu = UIMenu(title: "", options: .displayInline, children: [edit, delete])
      return UIMenu(title: favourite.title ?? favourite.url ?? "", identifier: nil, children: [urlMenu, favMenu])
    }
  }

  func collectionView(_ collectionView: UICollectionView, previewForHighlightingContextMenuWithConfiguration configuration: UIContextMenuConfiguration) -> UITargetedPreview? {
    guard let indexPath = configuration.identifier as? IndexPath,
      let cell = collectionView.cellForItem(at: indexPath) as? FavoriteCell
    else {
      return nil
    }
    return UITargetedPreview(view: cell.imageView)
  }

  func collectionView(_ collectionView: UICollectionView, previewForDismissingContextMenuWithConfiguration configuration: UIContextMenuConfiguration) -> UITargetedPreview? {
    guard let indexPath = configuration.identifier as? IndexPath,
      let cell = collectionView.cellForItem(at: indexPath) as? FavoriteCell
    else {
      return nil
    }
    return UITargetedPreview(view: cell.imageView)
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
