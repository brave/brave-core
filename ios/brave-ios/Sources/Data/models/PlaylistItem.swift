// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import CoreData
import Foundation
import Shared
import SwiftUI
import os.log

@objc(PlaylistItem)
final public class PlaylistItem: NSManagedObject, CRUD, Identifiable {
  @NSManaged public var cachedData: Data?
  @NSManaged public var dateAdded: Date
  @NSManaged public var duration: TimeInterval
  @NSManaged public var lastPlayedOffset: TimeInterval
  @NSManaged public var mediaSrc: String
  @NSManaged public var mimeType: String
  @NSManaged public var name: String
  @NSManaged public var order: Int32
  @NSManaged public var pageSrc: String
  @NSManaged public var pageTitle: String?
  @NSManaged public var uuid: String?
  @NSManaged public var playlistFolder: PlaylistFolder?

  static public func resolvingCachedData(_ cachedData: Data) async -> URL? {
    do {
      var isStale: Bool = false
      let url = try URL(resolvingBookmarkData: cachedData, bookmarkDataIsStale: &isStale)
      if FileManager.default.fileExists(atPath: url.path) {
        return url
      }
    } catch {
      return nil
    }
    return nil
  }

  @available(*, unavailable)
  public init() {
    fatalError("No Such Initializer: init()")
  }

  @available(*, unavailable)
  public init(context: NSManagedObjectContext) {
    fatalError("No Such Initializer: init(context:)")
  }

  @objc
  private override init(entity: NSEntityDescription, insertInto context: NSManagedObjectContext?) {
    super.init(entity: entity, insertInto: context)
  }

  public init(
    context: NSManagedObjectContext,
    name: String,
    pageTitle: String?,
    pageSrc: String,
    cachedData: Data,
    duration: TimeInterval,
    mimeType: String,
    mediaSrc: String
  ) {
    let entity = Self.entity(context)
    super.init(entity: entity, insertInto: context)
    self.name = name
    self.pageTitle = pageTitle
    self.pageSrc = pageSrc
    self.dateAdded = Date()
    self.cachedData = cachedData
    self.duration = duration
    self.lastPlayedOffset = 0.0
    self.mimeType = mimeType
    self.mediaSrc = mediaSrc
    self.order = .min
    self.uuid = UUID().uuidString
  }

  public var id: NSManagedObjectID {
    objectID
  }

  public class func frc() -> NSFetchedResultsController<PlaylistItem> {
    let context = DataController.viewContext
    let fetchRequest = NSFetchRequest<PlaylistItem>()
    fetchRequest.entity = PlaylistItem.entity(context)
    fetchRequest.fetchBatchSize = 20

    let orderSort = NSSortDescriptor(key: "order", ascending: true)
    let createdSort = NSSortDescriptor(key: "dateAdded", ascending: false)
    fetchRequest.sortDescriptors = [orderSort, createdSort]

    return NSFetchedResultsController(
      fetchRequest: fetchRequest,
      managedObjectContext: context,
      sectionNameKeyPath: nil,
      cacheName: nil
    )
  }

  public class func frc(parentFolder: PlaylistFolder?) -> NSFetchedResultsController<PlaylistItem> {
    let context = parentFolder?.managedObjectContext ?? DataController.viewContext
    let fetchRequest = NSFetchRequest<PlaylistItem>()
    fetchRequest.entity = PlaylistItem.entity(context)
    fetchRequest.fetchBatchSize = 20

    if let parentFolder = parentFolder {
      fetchRequest.predicate = NSPredicate(format: "playlistFolder.uuid == %@", parentFolder.uuid!)
    } else {
      fetchRequest.predicate = NSPredicate(format: "playlistFolder == nil")
    }

    let orderSort = NSSortDescriptor(key: "order", ascending: true)
    let createdSort = NSSortDescriptor(key: "dateAdded", ascending: false)
    fetchRequest.sortDescriptors = [orderSort, createdSort]

    return NSFetchedResultsController(
      fetchRequest: fetchRequest,
      managedObjectContext: context,
      sectionNameKeyPath: nil,
      cacheName: nil
    )
  }

  public class func allFoldersFRC() -> NSFetchedResultsController<PlaylistItem> {
    let context = DataController.viewContext
    let fetchRequest = NSFetchRequest<PlaylistItem>()
    fetchRequest.entity = PlaylistItem.entity(context)
    fetchRequest.fetchBatchSize = 20

    let orderSort = NSSortDescriptor(key: "order", ascending: true)
    let createdSort = NSSortDescriptor(key: "dateAdded", ascending: false)
    fetchRequest.sortDescriptors = [orderSort, createdSort]

    return NSFetchedResultsController(
      fetchRequest: fetchRequest,
      managedObjectContext: context,
      sectionNameKeyPath: nil,
      cacheName: nil
    )
  }

  public static func addItem(
    _ item: PlaylistInfo,
    folderUUID: String? = nil,
    cachedData: Data?,
    completion: (() -> Void)? = nil
  ) {
    DataController.perform(context: .new(inMemory: false), save: false) { context in
      let playlistItem = PlaylistItem(
        context: context,
        name: item.name,
        pageTitle: item.pageTitle,
        pageSrc: item.pageSrc,
        cachedData: cachedData ?? Data(),
        duration: item.duration,
        mimeType: item.mimeType,
        mediaSrc: item.src
      )
      playlistItem.order = item.order
      playlistItem.uuid = item.tagId
      playlistItem.playlistFolder = PlaylistFolder.getFolder(
        uuid: folderUUID ?? PlaylistFolder.savedFolderUUID,
        context: context
      )

      PlaylistItem.reorderItems(context: context, folderUUID: folderUUID)
      PlaylistItem.saveContext(context)

      DispatchQueue.main.async {
        completion?()
      }
    }
  }

  public static func addItems(
    _ items: [PlaylistInfo],
    folderUUID: String? = nil,
    cachedData: Data?,
    completion: (() -> Void)? = nil
  ) {
    DataController.perform(context: .new(inMemory: false), save: false) { context in
      let folder = PlaylistFolder.getFolder(
        uuid: folderUUID ?? PlaylistFolder.savedFolderUUID,
        context: context
      )
      for item in items {
        let playlistItem = PlaylistItem(
          context: context,
          name: item.name,
          pageTitle: item.pageTitle,
          pageSrc: item.pageSrc,
          cachedData: cachedData ?? Data(),
          duration: item.duration,
          mimeType: item.mimeType,
          mediaSrc: item.src
        )
        playlistItem.order = item.order
        playlistItem.uuid = item.tagId
        playlistItem.playlistFolder = folder
      }

      PlaylistItem.reorderItems(context: context, folderUUID: folderUUID)
      PlaylistItem.saveContext(context)

      DispatchQueue.main.async {
        completion?()
      }
    }
  }

  public static func reorderItems(
    in folder: PlaylistFolder,
    fromOffsets indexSet: IndexSet,
    toOffset offset: Int,
    completion: (() -> Void)? = nil
  ) {
    let frc = PlaylistItem.frc(parentFolder: folder)
    try? frc.performFetch()
    guard var objects = frc.fetchedObjects else {
      return
    }
    frc.managedObjectContext.perform {
      objects.move(fromOffsets: indexSet, toOffset: offset)

      for (order, item) in objects.enumerated().reversed() {
        item.order = Int32(order)
      }

      do {
        try frc.managedObjectContext.save()
      } catch {
        Logger.module.error("\(error.localizedDescription)")
      }

      DispatchQueue.main.async {
        completion?()
      }
    }
  }

  public static func addInMemoryItems(
    _ items: [PlaylistInfo],
    folderUUID: String,
    completion: (() -> Void)? = nil
  ) {
    DataController.perform(context: .existing(DataController.viewContextInMemory), save: false) {
      context in
      context.perform {
        let folder = PlaylistFolder.getFolder(uuid: folderUUID, context: context)

        items.forEach({ item in
          let playlistItem = PlaylistItem(
            context: context,
            name: item.name,
            pageTitle: item.pageTitle,
            pageSrc: item.pageSrc,
            cachedData: Data(),
            duration: item.duration,
            mimeType: item.mimeType,
            mediaSrc: item.src
          )
          playlistItem.order = item.order
          playlistItem.uuid = item.tagId
          playlistItem.playlistFolder = folder
        })

        PlaylistItem.reorderItems(context: context, folderUUID: folderUUID)
        PlaylistItem.saveContext(context)

        DispatchQueue.main.async {
          completion?()
        }
      }
    }
  }

  public static func saveInMemoryItemsToDisk(
    items: [PlaylistItem],
    folderUUID: String,
    completion: (() -> Void)? = nil
  ) {
    DataController.perform(context: .existing(DataController.viewContext), save: false) { context in
      let folder = PlaylistFolder.getFolder(uuid: folderUUID, context: context)

      items.forEach({ item in
        let playlistItem = PlaylistItem(
          context: context,
          name: item.name,
          pageTitle: item.pageTitle,
          pageSrc: item.pageSrc,
          cachedData: item.cachedData ?? Data(),
          duration: item.duration,
          mimeType: item.mimeType,
          mediaSrc: item.mediaSrc
        )
        playlistItem.order = item.order
        playlistItem.uuid = item.uuid
        playlistItem.playlistFolder = folder
      })

      PlaylistItem.reorderItems(context: context, folderUUID: folderUUID)

      // Issue #6243 The policy change is added to prevent merge conflicts
      // Occasionally saving context will give error
      // Fatal error: Error saving DB: Error Domain=NSCocoaErrorDomain Code=133020 "Could not merge changes."
      // Merge policy should change be changed to mergeByPropertyObjectTrumpMergePolicyType to prevent this
      let policy = context.mergePolicy
      context.mergePolicy = NSMergePolicy(merge: .mergeByPropertyObjectTrumpMergePolicyType)
      PlaylistItem.saveContext(context)
      context.mergePolicy = policy

      DispatchQueue.main.async {
        completion?()
      }
    }
  }

  public static func getItems(parentFolder: PlaylistFolder?) -> [PlaylistItem] {
    let predicate: NSPredicate
    if let parentFolder = parentFolder {
      predicate = NSPredicate(format: "playlistFolder.uuid == %@", parentFolder.uuid!)
    } else {
      predicate = NSPredicate(format: "playlistFolder == nil")
    }

    let orderSort = NSSortDescriptor(key: "order", ascending: true)
    let createdSort = NSSortDescriptor(key: "dateAdded", ascending: false)
    return PlaylistItem.all(
      where: predicate,
      sortDescriptors: [orderSort, createdSort],
      fetchBatchSize: 20
    ) ?? []
  }

  public static func getItems(pageSrc: String) -> [PlaylistItem] {
    return PlaylistItem.all(where: NSPredicate(format: "pageSrc == %@", pageSrc)) ?? []
  }

  public static func getItem(id: PlaylistItem.ID) -> PlaylistItem? {
    return PlaylistItem.first(where: NSPredicate(format: "self == %@", id))
  }

  public static func getItem(uuid: String) -> PlaylistItem? {
    return PlaylistItem.first(where: NSPredicate(format: "uuid == %@", uuid))
  }

  public class func all() -> [PlaylistItem] {
    all() ?? []
  }

  public class func count() -> Int {
    count() ?? 0
  }

  public static func itemExists(pageSrc: String) -> Bool {
    if let count = PlaylistItem.count(predicate: NSPredicate(format: "pageSrc == %@", pageSrc)),
      count > 0
    {
      return true
    }
    return false
  }

  public static func itemExists(uuid: String) -> Bool {
    if let count = PlaylistItem.count(predicate: NSPredicate(format: "uuid == %@", uuid)), count > 0
    {
      return true
    }
    return false
  }

  public static func cachedItem(cacheURL: URL) -> PlaylistItem? {
    return PlaylistItem.all()?.first(where: {
      var isStale = false

      if let cacheData = $0.cachedData,
        let url = try? URL(resolvingBookmarkData: cacheData, bookmarkDataIsStale: &isStale)
      {
        return url.path == cacheURL.path
      }
      return false
    })
  }

  public static func updateLastPlayed(
    itemId: String,
    pageSrc: String,
    lastPlayedOffset: TimeInterval
  ) {
    if itemExists(pageSrc: pageSrc) || itemExists(uuid: itemId) {
      DataController.perform(context: .new(inMemory: false), save: false) { context in
        if let existingItem = PlaylistItem.first(
          where: NSPredicate(format: "pageSrc == %@ OR uuid == %@", pageSrc, itemId),
          context: context
        ) {
          existingItem.lastPlayedOffset = lastPlayedOffset
        }

        PlaylistItem.saveContext(context)
      }
    }
  }

  public static func updateItem(_ item: PlaylistInfo, completion: (() -> Void)? = nil) {
    if itemExists(pageSrc: item.pageSrc) || itemExists(uuid: item.tagId) {
      DataController.perform(context: .new(inMemory: false), save: false) { context in
        if let existingItem = PlaylistItem.first(
          where: NSPredicate(format: "pageSrc == %@ OR uuid == %@", item.pageSrc, item.tagId),
          context: context
        ) {
          existingItem.name = item.name
          existingItem.pageTitle = item.pageTitle
          existingItem.pageSrc = item.pageSrc
          existingItem.duration = item.duration
          existingItem.lastPlayedOffset = item.lastPlayedOffset
          existingItem.mimeType = item.mimeType
          existingItem.mediaSrc = item.src
          existingItem.uuid = item.tagId
        }

        PlaylistItem.saveContext(context)

        DispatchQueue.main.async {
          completion?()
        }
      }
    } else {
      addItem(item, cachedData: nil, completion: completion)
    }
  }

  public static func updateItems(
    _ items: [PlaylistInfo],
    folderUUID: String? = nil,
    newETag: String? = nil,
    completion: (() -> Void)? = nil
  ) {
    DataController.perform(context: .new(inMemory: false), save: false) { context in
      let uuids = items.compactMap({ $0.tagId })
      let existingItems =
        PlaylistItem.all(where: NSPredicate(format: "uuid IN %@", uuids), context: context) ?? []
      let existingUUIDs = existingItems.compactMap({ $0.uuid })
      let newItems = items.filter({ !existingUUIDs.contains($0.tagId) })
      let folder = PlaylistFolder.getFolder(
        uuid: folderUUID ?? PlaylistFolder.savedFolderUUID,
        context: context
      )

      if let eTag = newETag {
        folder?.sharedFolderETag = eTag
      }

      existingItems.forEach { existingItem in
        items.forEach { item in
          if existingItem.uuid == item.tagId {
            existingItem.name = item.name
            existingItem.pageTitle = item.pageTitle
            existingItem.pageSrc = item.pageSrc
            existingItem.duration = item.duration
            existingItem.mimeType = item.mimeType
            existingItem.mediaSrc = item.src
            existingItem.playlistFolder = folder
          }
        }
      }

      newItems.forEach { item in
        let playlistItem = PlaylistItem(
          context: context,
          name: item.name,
          pageTitle: item.pageTitle,
          pageSrc: item.pageSrc,
          cachedData: Data(),
          duration: item.duration,
          mimeType: item.mimeType,
          mediaSrc: item.src
        )
        playlistItem.order = item.order
        playlistItem.uuid = item.tagId
        playlistItem.playlistFolder = folder
      }

      PlaylistItem.reorderItems(context: context, folderUUID: folderUUID)
      PlaylistItem.saveContext(context)

      DispatchQueue.main.async {
        completion?()
      }
    }
  }
  public static func updateCache(uuid: String, pageSrc: String, cachedData: Data?) {
    DataController.perform(context: .new(inMemory: false), save: true) { context in
      if let item = PlaylistItem.first(
        where: NSPredicate(format: "uuid == %@ OR pageSrc == %@", uuid, pageSrc),
        context: context
      ) {
        if let cachedData = cachedData, !cachedData.isEmpty {
          item.cachedData = cachedData
        } else {
          item.cachedData = nil
        }
      }
    }
  }

  public static func removeItem(uuid: String) {
    PlaylistItem.deleteAll(
      predicate: NSPredicate(format: "uuid == %@", uuid),
      context: .new(inMemory: false),
      includesPropertyValues: false
    )
  }

  public static func removeItems(_ items: [PlaylistInfo], completion: (() -> Void)? = nil) {
    var uuids = [String]()
    var mediaSrcs = [String]()

    items.forEach({
      if $0.tagId.isEmpty {
        mediaSrcs.append($0.src)
      } else {
        uuids.append($0.tagId)
      }
    })

    PlaylistItem.deleteAll(
      predicate: NSPredicate(format: "uuid IN %@", uuids),
      context: .new(inMemory: false),
      includesPropertyValues: false,
      completion: completion
    )

    if !mediaSrcs.isEmpty {
      // As a backup, we delete by media source (not page source)
      // This is more unique than pageSrc.
      // This is because you can add multiple items per page but they'd each have a different media source.
      PlaylistItem.deleteAll(
        predicate: NSPredicate(format: "mediaSrc IN %@", mediaSrcs),
        context: .new(inMemory: false),
        includesPropertyValues: false,
        completion: completion
      )
    }
  }

  public static func moveItems(
    items: [NSManagedObjectID],
    to folderUUID: String?,
    completion: (() -> Void)? = nil
  ) {
    DataController.perform { context in
      var folder: PlaylistFolder?
      if let folderUUID = folderUUID {
        folder = PlaylistFolder.getFolder(uuid: folderUUID, context: context)
      }

      let playlistItems = items.compactMap {
        try? context.existingObject(with: $0) as? PlaylistItem
      }
      playlistItems.forEach {
        $0.playlistFolder = folder
        folder?.playlistItems?.insert($0)
      }

      DispatchQueue.main.async {
        completion?()
      }
    }
  }

  // MARK: - Internal
  private static func reorderItems(context: NSManagedObjectContext, folderUUID: String?) {
    DataController.perform(context: .existing(context), save: true) { context in
      let request = NSFetchRequest<PlaylistItem>()
      request.entity = PlaylistItem.entity(context)
      request.fetchBatchSize = 20

      if let folderUUID = folderUUID {
        request.predicate = NSPredicate(format: "playlistFolder.uuid == %@", folderUUID)
      } else {
        request.predicate = NSPredicate(format: "playlistFolder == nil")
      }

      let orderSort = NSSortDescriptor(key: "order", ascending: true)
      let items = PlaylistItem.all(sortDescriptors: [orderSort], context: context) ?? []

      for (order, item) in items.enumerated() {
        item.order = Int32(order)
      }
    }
  }

  @nonobjc
  private class func fetchRequest() -> NSFetchRequest<PlaylistItem> {
    NSFetchRequest<PlaylistItem>(entityName: "PlaylistItem")
  }

  private static func entity(_ context: NSManagedObjectContext) -> NSEntityDescription {
    NSEntityDescription.entity(forEntityName: "PlaylistItem", in: context)!
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
