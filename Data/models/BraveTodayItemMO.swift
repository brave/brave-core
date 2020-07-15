// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import CoreData
import Shared

private let log = Logger.browserLogger

/// Bridge between `FeedItem` from Client target and Database model from `Data` target.
/// TODO: Perhaps we can move `FeedItem` into `Data` framework
public struct BraveTodayFeedItemBridge {
    let category: String
    let publishTime: Date
    let url: String?
    let imageURL: String?
    let title: String
    let itemDescription: String
    let contentType: String
    let publisherID: String
    let publisherLogo: String?
    let urlHash: String
}

public final class BraveTodayFeedItemMO: NSManagedObject, CRUD {
    // Properties we get from server
    @NSManaged var category: String
    @NSManaged var publishTime: Date
    @NSManaged var url: String?
    @NSManaged var imageURL: String?
    @NSManaged var title: String
    @NSManaged var itemDescription: String
    @NSManaged var contentType: String
    // TODO: Some publisher items will get moved to `BraveTodaySourceMO`
    @NSManaged var publisherID: String
    @NSManaged var publisherLogo: String?
    @NSManaged var urlHash: String
    @NSManaged var source: BraveTodaySourceMO?
    
    // Local properties
    @NSManaged var created: Date
    @NSManaged var viewed: Bool
    
    // MARK: Public interface
    
    public class func insert(item: BraveTodayFeedItemBridge) {
        insertInternal(category: item.category, publishTime: item.publishTime, url: item.url,
                       imageURL: item.imageURL, title: item.title,
                       itemDescription: item.itemDescription, contentType: item.contentType,
                       publisherID: item.publisherID,
                       publisherLogo: item.publisherLogo, urlHash: item.urlHash)
    }
    
    public class func insert(from list: [BraveTodayFeedItemBridge]) {
        // TODO: Use batch insert
        DataController.perform { context in
            list.forEach {
                insertInternal(category: $0.category, publishTime: $0.publishTime, url: $0.url,
                               imageURL: $0.imageURL, title: $0.title,
                               itemDescription: $0.itemDescription, contentType: $0.contentType,
                               publisherID: $0.publisherID,
                               publisherLogo: $0.publisherLogo, urlHash: $0.urlHash,
                               context: .existing(context))
            }
        }
    }
    
    public class func allItems(except hashUrls: [String]? = nil,
                               limit: Int = 0,
                               requiresImage: Bool,
                               contentType: String? = nil,
                               publisherID: String? = nil) -> [BraveTodayFeedItemMO] {
        let publishTimeSort = NSSortDescriptor(key: #keyPath(publishTime), ascending: false)
        
        // No need for special query, returning all available items.
        if !requiresImage && contentType == nil && publisherID == nil {
            return all(sortDescriptors: [publishTimeSort], fetchLimit: limit) ?? []
        }
        
        var viewedItemsPredicate: NSPredicate?
        var requiresImagePredicate: NSPredicate?
        var contentTypePredicate: NSPredicate?
        var publisherIDPredicate: NSPredicate?
        
        if let urlsToSkip = hashUrls {
            let urlHashKeyPath = #keyPath(BraveTodayFeedItemMO.urlHash)
            viewedItemsPredicate = NSPredicate(format: "NOT (\(urlHashKeyPath) IN %@)", urlsToSkip)
        }
        
        if requiresImage {
            let imageUrlKeyPath = #keyPath(BraveTodayFeedItemMO.imageURL)
            requiresImagePredicate = NSPredicate(format: "\(imageUrlKeyPath) != nil")
        }
        
        if let type = contentType {
            let contentTypeKeyPath = #keyPath(BraveTodayFeedItemMO.contentType)
            contentTypePredicate = NSPredicate(format: "\(contentTypeKeyPath) = %@", type)
        }
        
        if let pubID = publisherID {
            let publisherIDKeyPath = #keyPath(BraveTodayFeedItemMO.publisherID)
            publisherIDPredicate = NSPredicate(format: "\(publisherIDKeyPath) = %@", pubID)
        }
        
        let predicates = [requiresImagePredicate, contentTypePredicate,
                          publisherIDPredicate, viewedItemsPredicate].compactMap { $0 }
        let compoundPredicate = NSCompoundPredicate(type: .and, subpredicates: predicates)
        
        return all(where: compoundPredicate, sortDescriptors: [publishTimeSort], fetchLimit: limit) ?? []
    }
    
    public class func get(with url: String) -> BraveTodayFeedItemMO? {
        let urlKeyPath = #keyPath(BraveTodayFeedItemMO.url)
        let predicate = NSPredicate(format: "\(urlKeyPath) == %@", url)
        return first(where: predicate)
    }
    
    public class func updateFeedRecord(urlHash: String, session: String) {
        // TODO: We may not store session id in the database.
    }
    
    public class func updateFeedRecords(urlHashes: [String], session: String) {
        // TODO: We may not store session id in the database.
    }
    
    public class func markFeedRecordAsRead(urlHashe: String, read: Bool) {
        
    }
    
    public class func deleteAllItems(publisherId: String? = nil) {
        var predicate: NSPredicate?
        if let pubId = publisherId {
            let publisherIDKeyPath = #keyPath(BraveTodayFeedItemMO.publisherID)
            predicate = NSPredicate(format: "\(publisherIDKeyPath) == %@", pubId)
        }
        
        deleteAll(predicate: predicate)
    }
    
    public class func delete(with urlHash: String) {
        let urlHashKeyPath = #keyPath(BraveTodayFeedItemMO.urlHash)
        let predicate = NSPredicate(format: "\(urlHashKeyPath) == %@", urlHash)
        let record = first(where: predicate)
        
        record?.delete()
    }
    
    // MARK: Internal implementations
    
    class func insertInternal(category: String, publishTime: Date, url: String?, 
                              imageURL: String?, title: String, itemDescription: String, contentType: String,
                              publisherID: String, publisherLogo: String?,
                              urlHash: String, context: WriteContext = .new(inMemory: false)) {
        
        DataController.perform(context: context) { context in
            let sourcePublisherIdKeyPath = #keyPath(BraveTodaySourceMO.publisherID)
            let sourcePublisherIdPredicate =
                NSPredicate(format: "\(sourcePublisherIdKeyPath) == %@", publisherID)
            guard let source =
                    BraveTodaySourceMO.first(where: sourcePublisherIdPredicate, context: context) else {
                log.warning("Could not insert feed item, no publisher with id: \(publisherID) found.")
                return
            }
            
            let item = BraveTodayFeedItemMO(entity: entity(in: context), insertInto: context)
            
            item.category = category
            item.publishTime = publishTime
            item.url = url
            item.imageURL = imageURL
            item.title = title
            item.itemDescription = itemDescription
            item.contentType = contentType
            item.publisherID = publisherID
            item.publisherLogo = publisherLogo
            item.urlHash = urlHash
            item.created = Date()
            item.source = source
        }
    }
    
    private class func entity(in context: NSManagedObjectContext) -> NSEntityDescription {
        NSEntityDescription.entity(forEntityName: "BraveTodayFeedItemMO", in: context)!
    }
}
