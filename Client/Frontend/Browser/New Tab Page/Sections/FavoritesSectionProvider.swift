// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI
import Data
import CoreData
import Shared

private let log = Logger.browserLogger

enum BookmarksAction {
    case opened(inNewTab: Bool = false, switchingToPrivateMode: Bool = false)
    case edited
}

class FavoritesSectionProvider: NSObject, NTPObservableSectionProvider {
    var sectionDidChange: (() -> Void)?
    var action: (Bookmark, BookmarksAction) -> Void
    var legacyLongPressAction: (UIAlertController) -> Void
    
    private var frc: NSFetchedResultsController<Bookmark>
    
    init(action: @escaping (Bookmark, BookmarksAction) -> Void,
         legacyLongPressAction: @escaping (UIAlertController) -> Void) {
        self.action = action
        self.legacyLongPressAction = legacyLongPressAction
        
        frc = Bookmark.frc(forFavorites: true, parentFolder: nil)
        super.init()
        frc.fetchRequest.fetchLimit = 10
        frc.delegate = self
        
        do {
            try frc.performFetch()
        } catch {
            log.error("Favorites fetch error")
        }
    }
    
    static var defaultIconSize = CGSize(width: 82, height: FavoriteCell.height(forWidth: 82))
    static var largerIconSize = CGSize(width: 100, height: FavoriteCell.height(forWidth: 100))
    
    /// The number of times that each row contains
    static func numberOfItems(in collectionView: UICollectionView, availableWidth: CGFloat) -> Int {
        /// Two considerations:
        /// 1. icon size minimum
        /// 2. trait collection
        /// 3. orientation ("is landscape")
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
        return min(fetchedCount, Self.numberOfItems(in: collectionView, availableWidth: fittingSizeForCollectionView(collectionView, section: section).width))
    }
    
    func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
        // swiftlint:disable:next force_cast
        let cell = collectionView.dequeueReusableCell(withReuseIdentifier: FavoriteCell.identifier, for: indexPath) as! FavoriteCell
        let fav = frc.object(at: IndexPath(item: indexPath.item, section: 0))
        cell.textLabel.appearanceTextColor = .white
        cell.textLabel.text = fav.displayTitle ?? fav.url
        if let url = fav.url?.asURL {
            // All favorites should have domain's, but it was noticed at one
            // point that this wasn't the case, so for future bug-tracking
            // assert if its not found.
            assert(fav.domain != nil, "Domain should exist for all favorites")
            // The domain for the favorite is required for pulling cached
            // favicon info. Since all favorites should have persisted
            // Domain's, we leave `persistent` as true
            let domain = fav.domain ?? Domain.getOrCreate(forUrl: url, persistent: true)
            cell.imageView.monogramFallbackCharacter = fav.title?.first
            cell.imageView.domain = domain
        }
        cell.accessibilityLabel = cell.textLabel.text
        cell.longPressHandler = { [weak self] cell in
            let alert = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
            
            let edit = UIAlertAction(title: Strings.editBookmark, style: .default) { (action) in
                self?.action(fav, .edited)
            }
            let delete = UIAlertAction(title: Strings.removeFavorite, style: .destructive) { (action) in
                fav.delete()
            }
            
            alert.addAction(edit)
            alert.addAction(delete)
            
            alert.popoverPresentationController?.sourceView = cell
            alert.popoverPresentationController?.permittedArrowDirections = [.down, .up]
            alert.addAction(UIAlertAction(title: Strings.close, style: .cancel, handler: nil))
            
            UIImpactFeedbackGenerator(style: .medium).bzzt()
            self?.legacyLongPressAction(alert)
        }
        return cell
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
    
    @available(iOS 13.0, *)
    func collectionView(_ collectionView: UICollectionView, contextMenuConfigurationForItemAt indexPath: IndexPath, point: CGPoint) -> UIContextMenuConfiguration? {
        guard let favourite = frc.fetchedObjects?[indexPath.item] else { return nil }
        return UIContextMenuConfiguration(identifier: indexPath as NSCopying, previewProvider: nil) { _ -> UIMenu? in
            let openInNewTab = UIAction(title: Strings.openNewTabButtonTitle, handler: UIAction.deferredActionHandler { _ in
                self.action(favourite, .opened(inNewTab: true, switchingToPrivateMode: false))
            })
            let edit = UIAction(title: Strings.editBookmark, handler: UIAction.deferredActionHandler { _ in
                self.action(favourite, .edited)
            })
            let delete = UIAction(title: Strings.removeFavorite, attributes: .destructive, handler: UIAction.deferredActionHandler { _ in
                favourite.delete()
            })
            
            var urlChildren: [UIAction] = [openInNewTab]
            if !PrivateBrowsingManager.shared.isPrivateBrowsing {
                let openInNewPrivateTab = UIAction(title: Strings.openNewPrivateTabButtonTitle, handler: UIAction.deferredActionHandler { _ in
                    self.action(favourite, .opened(inNewTab: true, switchingToPrivateMode: true))
                })
                urlChildren.append(openInNewPrivateTab)
            }
            
            let urlMenu = UIMenu(title: "", options: .displayInline, children: urlChildren)
            let favMenu = UIMenu(title: "", options: .displayInline, children: [edit, delete])
            return UIMenu(title: favourite.title ?? favourite.url ?? "", identifier: nil, children: [urlMenu, favMenu])
        }
    }
    
    @available(iOS 13.0, *)
    func collectionView(_ collectionView: UICollectionView, previewForHighlightingContextMenuWithConfiguration configuration: UIContextMenuConfiguration) -> UITargetedPreview? {
        guard let indexPath = configuration.identifier as? IndexPath,
            let cell = collectionView.cellForItem(at: indexPath) as? FavoriteCell else {
                return nil
        }
        return UITargetedPreview(view: cell.imageView)
    }
    
    @available(iOS 13.0, *)
    func collectionView(_ collectionView: UICollectionView, previewForDismissingContextMenuWithConfiguration configuration: UIContextMenuConfiguration) -> UITargetedPreview? {
        guard let indexPath = configuration.identifier as? IndexPath,
            let cell = collectionView.cellForItem(at: indexPath) as? FavoriteCell else {
                return nil
        }
        return UITargetedPreview(view: cell.imageView)
    }
}

extension FavoritesSectionProvider: NSFetchedResultsControllerDelegate {
    func controllerDidChangeContent(_ controller: NSFetchedResultsController<NSFetchRequestResult>) {
        try? frc.performFetch()
        sectionDidChange?()
    }
}
