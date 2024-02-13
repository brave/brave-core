// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import CoreData
import Shared
import os.log

@objc(PlaylistFolder)
final public class PlaylistFolder: NSManagedObject, CRUD, Identifiable {
  @NSManaged public var uuid: String?
  @NSManaged public var title: String?
  @NSManaged public var order: Int32
  @NSManaged public var dateAdded: Date?
  @NSManaged public var sharedFolderId: String?
  @NSManaged public var sharedFolderUrl: String?
  @NSManaged public var sharedFolderETag: String?
  @NSManaged public var creatorName: String?
  @NSManaged public var creatorLink: String?
  @NSManaged public var playlistItems: Set<PlaylistItem>?
  public static let savedFolderUUID = "7B6CC019-8946-4182-ACE8-42FE7B704C43"

  public var id: String {
    uuid ?? UUID().uuidString
  }

  public class func frc(savedFolder: Bool, sharedFolders: Bool) -> NSFetchedResultsController<PlaylistFolder> {
    let context = DataController.viewContext
    let fetchRequest = NSFetchRequest<PlaylistFolder>()
    fetchRequest.entity = PlaylistFolder.entity(context)
    fetchRequest.fetchBatchSize = 20

    let orderSort = NSSortDescriptor(key: "order", ascending: true)
    let createdSort = NSSortDescriptor(key: "dateAdded", ascending: false)
    fetchRequest.sortDescriptors = [orderSort, createdSort]

    if savedFolder {
      fetchRequest.predicate = NSPredicate(format: "uuid == %@", savedFolderUUID)
    } else {
      fetchRequest.predicate = NSCompoundPredicate(andPredicateWithSubpredicates: [
        NSPredicate(format: "uuid != %@", savedFolderUUID),
        NSPredicate(format: sharedFolders ? "sharedFolderId != nil" : "sharedFolderId == nil")
      ])
    }

    return NSFetchedResultsController(
      fetchRequest: fetchRequest, managedObjectContext: context,
      sectionNameKeyPath: nil, cacheName: nil)
  }

  public static func addFolder(title: String, uuid: String? = nil, completion: ((_ uuid: String) -> Void)? = nil) {
    DataController.perform(context: .new(inMemory: false), save: false) { context in
      var folderId: String
      if let uuid = uuid, !uuid.isEmpty {
        folderId = uuid
      } else {
        folderId = UUID().uuidString
      }

      let playlistFolder = PlaylistFolder(context: context)
      playlistFolder.title = title
      playlistFolder.dateAdded = Date()
      playlistFolder.order = Int32.min
      playlistFolder.uuid = folderId
      playlistFolder.sharedFolderId = nil
      playlistFolder.sharedFolderUrl = nil
      playlistFolder.sharedFolderETag = nil

      PlaylistFolder.reorderItems(context: context)
      PlaylistFolder.saveContext(context)
      
      DispatchQueue.main.async {
        completion?(folderId)
      }
    }
  }
  
  public static func addInMemoryFolder(title: String, creatorName: String, creatorLink: String?, sharedFolderId: String, sharedFolderUrl: String?, sharedFolderETag: String?, completion: ((_ folder: PlaylistFolder, _ uuid: String) -> Void)? = nil) {
    DataController.perform(context: .existing(DataController.viewContextInMemory), save: false) { context in
      context.perform {
        let folderId = UUID().uuidString
        let playlistFolder = PlaylistFolder(context: context)
        playlistFolder.title = title
        playlistFolder.creatorName = creatorName
        playlistFolder.creatorLink = creatorLink
        playlistFolder.dateAdded = Date()
        playlistFolder.order = Int32.min
        playlistFolder.uuid = folderId
        playlistFolder.sharedFolderId = sharedFolderId
        playlistFolder.sharedFolderUrl = sharedFolderUrl
        playlistFolder.sharedFolderETag = sharedFolderETag
        
        PlaylistFolder.reorderItems(context: context)
        PlaylistFolder.saveContext(context)
        
        DispatchQueue.main.async {
          completion?(playlistFolder, folderId)
        }
      }
    }
  }
  
  public static func saveInMemoryFolderToDisk(folder: PlaylistFolder, completion: ((_ uuid: String) -> Void)? = nil) {
    DataController.perform(context: .existing(DataController.viewContext), save: false) { context in
      
      let folderId = folder.uuid ?? UUID().uuidString
      let playlistFolder = PlaylistFolder(context: context)
      playlistFolder.uuid = folder.uuid
      playlistFolder.title = folder.title
      playlistFolder.order = Int32.min
      playlistFolder.dateAdded = folder.dateAdded
      playlistFolder.sharedFolderId = folder.sharedFolderId
      playlistFolder.sharedFolderUrl = folder.sharedFolderUrl
      playlistFolder.sharedFolderETag = folder.sharedFolderETag
      playlistFolder.creatorName = folder.creatorName
      playlistFolder.creatorLink = folder.creatorLink
      
      PlaylistFolder.reorderItems(context: context)
      
      // Issue #6243 The policy change is added to prevent merge conflicts
      // Occasionally saving context will give error
      // Fatal error: Error saving DB: Error Domain=NSCocoaErrorDomain Code=133020 "Could not merge changes."
      // Merge policy should change be changed to mergeByPropertyObjectTrumpMergePolicyType to prevent this
      let policy = context.mergePolicy
      context.mergePolicy = NSMergePolicy(merge: .mergeByPropertyObjectTrumpMergePolicyType)
      PlaylistFolder.saveContext(context)
      context.mergePolicy = policy
      
      DispatchQueue.main.async {
        completion?(folderId)
      }
    }
  }

  public static func getOtherFoldersCount() -> Int {
    PlaylistFolder.count(predicate: NSPredicate(format: "uuid != %@", PlaylistFolder.savedFolderUUID)) ?? 0
  }

  public static func getFolder(uuid: String, context: NSManagedObjectContext? = nil) -> PlaylistFolder? {
    PlaylistFolder.first(where: NSPredicate(format: "uuid == %@", uuid), context: context ?? DataController.viewContext)
  }
  
  public static func getSharedFolder(sharedFolderUrl: String, context: NSManagedObjectContext? = nil) -> PlaylistFolder? {
    PlaylistFolder.first(where: NSPredicate(format: "sharedFolderUrl == %@", sharedFolderUrl), context: context ?? DataController.viewContext)
  }
  
  public static func getSharedFolders(context: NSManagedObjectContext? = nil) -> [PlaylistFolder] {
    PlaylistFolder.all(where: NSPredicate(format: "sharedFolderUrl != nil"), context: context ?? DataController.viewContext) ?? []
  }

  public static func removeFolder(_ uuid: String, completion: (() -> Void)? = nil) {
    PlaylistFolder.deleteAll(
      predicate: NSPredicate(format: "uuid == %@", uuid),
      includesPropertyValues: false,
      completion: completion)
  }

  public static func updateFolder(folderID: NSManagedObjectID, _ update: @escaping (Result<PlaylistFolder, Error>) -> Void) {
    DataController.perform(context: .new(inMemory: false), save: true) { context in
      do {
        guard let folder = try context.existingObject(with: folderID) as? PlaylistFolder else {
          fatalError("folder ID \(folderID) is not a PlaylistFolder")
        }

        update(.success(folder))
      } catch {
        update(.failure(error))
      }
    }
  }

  // MARK: - Internal
  private static func reorderItems(context: NSManagedObjectContext) {
    DataController.perform(context: .existing(context), save: true) { context in
      let request = NSFetchRequest<PlaylistFolder>()
      request.entity = PlaylistFolder.entity(context)
      request.fetchBatchSize = 20

      let orderSort = NSSortDescriptor(key: "order", ascending: true)
      let items = PlaylistFolder.all(sortDescriptors: [orderSort], context: context) ?? []

      for (order, item) in items.enumerated() {
        item.order = Int32(order)
      }
    }
  }

  @nonobjc
  private class func fetchRequest() -> NSFetchRequest<PlaylistFolder> {
    NSFetchRequest<PlaylistFolder>(entityName: "PlaylistFolder")
  }

  private static func entity(_ context: NSManagedObjectContext) -> NSEntityDescription {
    NSEntityDescription.entity(forEntityName: "PlaylistFolder", in: context)!
  }

  private static func saveContext(_ context: NSManagedObjectContext) {
    if context.concurrencyType == .mainQueueConcurrencyType {
      Logger.module.warning("Writing to view context, this should be avoided.")
    }

    if context.hasChanges {
      do {
        try context.save()
      } catch {
        assertionFailure("Error saving DB: \(error.localizedDescription)")
      }
    }
  }
}
