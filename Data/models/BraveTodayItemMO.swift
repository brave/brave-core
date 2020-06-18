// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import CoreData

/// Bridge between `FeedItem` from Client target and Database model from `Data` target.
/// TODO: Perhaps we can move `FeedItem` into `Data` framework
public struct BraveTodayFeedItemBridge {
    let category: String
    let publishTime: Date
    let url: String?
    let domain: String?
    let imageURL: String?
    let title: String
    let itemDescription: String
    let contentType: String
    let publisherID: String
    let publisherName: String
    let publisherLogo: String?
}

public final class BraveTodayFeedItemMO: NSManagedObject, CRUD {
    // What we get from the server
    @NSManaged var category: String
    @NSManaged var publishTime: Date
    @NSManaged var url: String?
    @NSManaged var domain: String?
    @NSManaged var imageURL: String?
    @NSManaged var title: String
    @NSManaged var itemDescription: String
    @NSManaged var contentType: String
    @NSManaged var publisherID: String
    @NSManaged var publisherName: String
    @NSManaged var publisherLogo: String?
    
    // Local properties
    // TODO: Add them
    
    // MARK: Public interface
    
    public class func insert(item: BraveTodayFeedItemBridge) {
        insertInternal(category: item.category, publishTime: item.publishTime, url: item.url,
                       domain: item.domain, imageURL: item.imageURL, title: item.title,
                       itemDescription: item.itemDescription, contentType: item.contentType,
                       publisherID: item.publisherID, publisherName: item.publisherName,
                       publisherLogo: item.publisherLogo)
    }
    
    public class func insert(from list: [BraveTodayFeedItemBridge]) {
        DataController.perform { context in
            list.forEach {
                insertInternal(category: $0.category, publishTime: $0.publishTime, url: $0.url,
                               domain: $0.domain, imageURL: $0.imageURL, title: $0.title,
                               itemDescription: $0.itemDescription, contentType: $0.contentType,
                               publisherID: $0.publisherID, publisherName: $0.publisherName,
                               publisherLogo: $0.publisherLogo)
            }
        }
    }
    
    public class func allItems() -> [BraveTodayFeedItemMO] {
        all() ?? []
    }
    
    public class func deleteAllItems() {
        deleteAll()
    }
    
    // MARK: Internal implementations
    
    class func insertInternal(category: String, publishTime: Date, url: String?, domain: String?,
                              imageURL: String?, title: String, itemDescription: String, contentType: String,
                              publisherID: String, publisherName: String, publisherLogo: String?,
                              context: WriteContext = .new(inMemory: false)) {
        
        DataController.perform(context: context) { context in
            // TODO: Check if we have to pass entity or just context is enough
            //BraveTodayFeedItemMO(entity:, insertInto:)
            
            let item = BraveTodayFeedItemMO(context: context)
            
            item.category = category
            item.publishTime = publishTime
            item.url = url
            item.domain = domain
            item.imageURL = imageURL
            item.title = title
            item.itemDescription = itemDescription
            item.contentType = contentType
            item.publisherID = publisherID
            item.publisherName = publisherName
            item.publisherLogo = publisherLogo
        }
    }
}
