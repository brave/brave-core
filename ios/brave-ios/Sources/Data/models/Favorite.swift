/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import CoreData
import Foundation
import Shared
import Storage
import os.log

public protocol WebsitePresentable {
  var title: String? { get }
  var url: String? { get }
}

/// Note: This class is named as `Bookmark` in our core data model due to sync v1 legacy..
public final class Favorite: NSManagedObject, WebsitePresentable, CRUD {
  @NSManaged public var title: String?
  @NSManaged public var customTitle: String?
  @NSManaged public var url: String?
  @NSManaged public var lastVisited: Date?
  @NSManaged public var created: Date?
  @NSManaged public var order: Int16

  @NSManaged public var domain: Domain?

  // MARK: Legacy
  /// Pre sync v2 this object could be either a bookmark or a favorite, this flag was used to store this info.
  @NSManaged public var isFavorite: Bool
  /// Legacy: this property is not used anymore except for migration.
  @NSManaged public var isFolder: Bool
  /// Unused
  @NSManaged public var tags: [String]?
  /// Unused
  @NSManaged public var visits: Int32

  @available(*, deprecated, message: "This is a sync v1 property and is not used anymore")
  @NSManaged public var syncDisplayUUID: String?
  @available(*, deprecated, message: "This is a sync v1 property and is not used anymore")
  @NSManaged public var syncParentDisplayUUID: String?
  @available(*, deprecated, message: "This is a sync v1 property and is not used anymore")
  @NSManaged public var syncOrder: String?

  private static let isFavoritePredicate = NSPredicate(format: "isFavorite == true")

  // MARK: - Public interface

  // MARK: Create

  public class func add(from list: [(url: URL, title: String)]) {
    DataController.perform { context in
      list.forEach {
        addInternal(url: $0.url, title: $0.title, isFavorite: true, context: .existing(context))
      }
    }
  }

  public class func add(url: URL, title: String?) {
    addInternal(url: url, title: title, isFavorite: true)
  }

  // MARK: Read

  public var displayTitle: String? {
    if let custom = customTitle, !custom.isEmpty {
      return customTitle
    }

    if let t = title, !t.isEmpty {
      return title
    }

    // Want to return nil so less checking on frontend
    return nil
  }

  public static func frc() -> NSFetchedResultsController<Favorite> {
    let context = DataController.viewContext
    let fetchRequest = NSFetchRequest<Favorite>()

    fetchRequest.entity = Favorite.entity(context: context)
    fetchRequest.fetchBatchSize = 20

    let orderSort = NSSortDescriptor(key: "order", ascending: true)
    let createdSort = NSSortDescriptor(key: "created", ascending: false)
    fetchRequest.sortDescriptors = [orderSort, createdSort]

    fetchRequest.predicate = isFavoritePredicate

    return NSFetchedResultsController(
      fetchRequest: fetchRequest, managedObjectContext: context,
      sectionNameKeyPath: nil, cacheName: nil)
  }

  public class func contains(url: URL) -> Bool {
    let predicate = NSPredicate(format: "url == %@ AND isFavorite == true", url.absoluteString)

    return (count(predicate: predicate) ?? 0) > 0
  }

  public class var hasFavorites: Bool {
    guard let favoritesCount = count(predicate: isFavoritePredicate) else { return false }
    return favoritesCount > 0
  }

  public class var allFavorites: [Favorite] {
    return all(where: isFavoritePredicate) ?? []
  }

  public class func get(with objectID: NSManagedObjectID) -> Favorite? {
    DataController.viewContext.object(with: objectID) as? Favorite
  }

  // MARK: Update

  public func update(customTitle: String?, url: String?) {
    if !hasTitle(customTitle) { return }
    updateInternal(customTitle: customTitle, url: url)
  }

  // Title can't be empty.
  private func hasTitle(_ title: String?) -> Bool {
    return title?.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty == false
  }

  /// WARNING: This method deletes all current favorites and replaces them with new one from the array.
  public class func forceOverwriteFavorites(with favorites: [(url: URL, title: String)]) {
    DataController.perform { context in
      Favorite.deleteAll(predicate: isFavoritePredicate, context: .existing(context))

      favorites.forEach {
        addInternal(
          url: $0.url, title: $0.title, isFavorite: true,
          context: .existing(context))
      }
    }
  }

  /// Passing in `isInteractiveDragReorder` will force the write to happen on
  /// the main view context. Defaults to `false`
  public class func reorder(
    sourceIndexPath: IndexPath,
    destinationIndexPath: IndexPath,
    isInteractiveDragReorder: Bool = false
  ) {
    if destinationIndexPath.row == sourceIndexPath.row {
      Logger.module.error("Source and destination bookmarks are the same!")
      return
    }

    // If we're doing an interactive drag reorder then we want to make sure
    // to do the reorder on main queue so the underlying dataset & FRC is
    // updated immediately so the animation does not glitch out on drop.
    //
    // Doing it on a background thread will cause the favorites overlay
    // to temporarely show the old items and require a full-table refresh
    let context: WriteContext = isInteractiveDragReorder ? .existing(DataController.viewContext) : .new(inMemory: false)

    DataController.perform(context: context) { context in
      let destinationIndex = destinationIndexPath.row
      let source = sourceIndexPath.row

      var allFavorites = Favorite.getAllFavorites(context: context).sorted { $0.order < $1.order }

      if let sourceIndex = allFavorites.firstIndex(where: { $0.order == source }) {
        let removedItem = allFavorites.remove(at: sourceIndex)
        allFavorites.insert(removedItem, at: destinationIndex)
      }

      // Update order of all favorites that have changed.
      for (index, element) in allFavorites.enumerated() where index != element.order {
        element.order = Int16(index)
      }

      if isInteractiveDragReorder && context.hasChanges {
        do {
          assert(Thread.isMainThread)
          try context.save()
        } catch {
          Logger.module.error("performTask save error: \(error.localizedDescription)")
        }
      }
    }
  }

  // MARK: Delete

  public func delete(context: WriteContext? = nil) {
    deleteInternal(context: context ?? .new(inMemory: false))
  }
}

// MARK: - Internal implementations
extension Favorite {
  /// Favorites are named `Bookmark` due to legacy reasons.
  /// Pre sync-v2 we used this class for both bookmarks and favorites
  static func entity(context: NSManagedObjectContext) -> NSEntityDescription {
    return NSEntityDescription.entity(forEntityName: "Bookmark", in: context)!
  }

  // MARK: Create

  /// - parameter completion: Returns object id associated with this object.
  /// IMPORTANT: this id might change after the object has been saved to persistent store. Better to use it within one context.
  class func addInternal(
    url: URL?,
    title: String?,
    customTitle: String? = nil,
    isFavorite: Bool,
    save: Bool = true,
    context: WriteContext = .new(inMemory: false)
  ) {

    DataController.perform(
      context: context, save: save,
      task: { context in
        let bk = Favorite(entity: entity(context: context), insertInto: context)

        let location = url?.absoluteString

        bk.url = location
        bk.title = title
        bk.customTitle = customTitle
        bk.isFavorite = isFavorite
        bk.created = Date()
        bk.lastVisited = bk.created

        if let location = location, let url = URL(string: location) {
          bk.domain = Domain.getOrCreateInternal(
            url, context: context,
            saveStrategy: .delayedPersistentStore)
        }

        let favorites = getAllFavorites(context: context)

        // First fav is zero, then we increment all others.
        if favorites.count > 1, let lastOrder = favorites.map(\.order).max() {
          bk.order = lastOrder + 1
        }
      })
  }

  // MARK: Update

  private func updateInternal(
    customTitle: String?, url: String?, save: Bool = true,
    context: WriteContext = .new(inMemory: false)
  ) {

    DataController.perform(context: context) { context in
      guard let bookmarkToUpdate = context.object(with: self.objectID) as? Favorite else { return }

      // See if there has been any change
      if bookmarkToUpdate.customTitle == customTitle && bookmarkToUpdate.url == url {
        return
      }

      bookmarkToUpdate.customTitle = customTitle
      bookmarkToUpdate.title = customTitle ?? bookmarkToUpdate.title

      if let u = url, !u.isEmpty {
        bookmarkToUpdate.url = url
        if let theURL = URL(string: u) {
          bookmarkToUpdate.domain =
            Domain.getOrCreateInternal(
              theURL, context: context,
              saveStrategy: .delayedPersistentStore)
        } else {
          bookmarkToUpdate.domain = nil
        }
      }
    }
  }

  // MARK: Read

  private static func getAllFavorites(context: NSManagedObjectContext? = nil) -> [Favorite] {
    let predicate = NSPredicate(format: "isFavorite == YES")

    return all(where: predicate, context: context ?? DataController.viewContext) ?? []
  }

  // MARK: Delete

  private func deleteInternal(context: WriteContext = .new(inMemory: false)) {
    func deleteFromStore(context: WriteContext) {
      DataController.perform(context: context) { context in
        let objectOnContext = context.object(with: self.objectID)
        context.delete(objectOnContext)
      }
    }

    if isFavorite { deleteFromStore(context: context) }
    deleteFromStore(context: context)
  }
}

// MARK: - Comparable
extension Favorite: Comparable {
  public static func < (lhs: Favorite, rhs: Favorite) -> Bool {
    return lhs.order < rhs.order
  }
}
