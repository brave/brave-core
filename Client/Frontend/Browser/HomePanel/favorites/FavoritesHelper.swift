/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import CoreData
import Shared
import Storage
import Data

private let log = Logger.browserLogger

/// A set of methods related to managing favorites. Most are just wrappers over Bookmark model.
struct FavoritesHelper {
    // Indicates if favorites have been initialized.
    static let initPrefsKey = "FavoritesHelperInitPrefsKey"

    static func frc() -> NSFetchedResultsController<NSFetchRequestResult> {
        let context = DataController.mainThreadContext
        let fetchRequest = NSFetchRequest<NSFetchRequestResult>()

        fetchRequest.entity = Bookmark.entity(context: context)
        fetchRequest.fetchBatchSize = 20

        // We always want favorites folder to be on top, in the first section.
        let orderSort = NSSortDescriptor(key:"order", ascending: true)
        let createdSort = NSSortDescriptor(key:"created", ascending: false)
        fetchRequest.sortDescriptors = [orderSort, createdSort]

        fetchRequest.predicate = NSPredicate(format: "isFavorite == YES")

        return NSFetchedResultsController(fetchRequest: fetchRequest, managedObjectContext:context,
                                          sectionNameKeyPath: nil, cacheName: nil)
    }

    // MARK: - Favorites initialization
    static func addDefaultFavorites() {
        PreloadedFavorites.getList().forEach { fav in
            Bookmark.add(url: fav.url, title: fav.title, isFavorite: true)
        }
    }

    static func convertToBookmarks(_ sites: [Site]) {
        sites.forEach { site in
            if let url = try? site.url.asURL() {
                Bookmark.add(url: url, title: url.normalizedHost ?? site.url, isFavorite: true)
            }
        }
    }

    static func add(url: URL, title: String?, color: UIColor?) {
        Bookmark.add(url: url, title: title, isFavorite: true, color: color)
    }

    static func isAlreadyAdded(_ url: URL) -> Bool{
        return Bookmark.contains(url: url, getFavorites: true, context: DataController.mainThreadContext)
    }
    
    static func fallbackIcon(withLetter letter: String, color: UIColor, andSize iconSize: CGSize) -> UIImage {
        let renderer = UIGraphicsImageRenderer(size: iconSize)
        return  renderer.image { ctx in
            let rectangle = CGRect(x: 0, y: 0, width: iconSize.width, height: iconSize.height)
            
            ctx.cgContext.addRect(rectangle)
            ctx.cgContext.setFillColor(color.cgColor)
            ctx.cgContext.drawPath(using: .fillStroke)
            
            let paragraphStyle = NSMutableParagraphStyle()
            paragraphStyle.alignment = .center
            
            let attrs = [NSAttributedStringKey.font: UIFont(name: "HelveticaNeue-Thin", size: iconSize.height-90) ?? UIFont.systemFont(ofSize: iconSize.height-90, weight: UIFont.Weight.thin),
                         NSAttributedStringKey.paragraphStyle: paragraphStyle,
                         NSAttributedStringKey.backgroundColor: UIColor.clear]
            
            let string: NSString = NSString(string: letter.uppercased())
            let size = string.size(withAttributes: attrs)
            string.draw(at: CGPoint(x: (iconSize.width-size.width)/2, y: (iconSize.height-size.height)/2), withAttributes: attrs)
        }
    }
}
