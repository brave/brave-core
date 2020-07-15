// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import CoreData

public final class BraveTodaySourceMO: NSManagedObject, CRUD {
    @NSManaged var enabled: Bool
    @NSManaged var publisherID: String
    @NSManaged var publisherLogo: String?
    @NSManaged var publisherName: String
    @NSManaged var items: [BraveTodayFeedItemMO]
    
    public class func insert(publisherID: String, publisherLogo: String?, publisherName: String) {
        insertInternal(publisherID: publisherID, publisherLogo: publisherLogo, publisherName: publisherName)
    }
    
    public class func insert(from list: [(publisherID: String, publisherLogo: String?, publisherName: String)]) {
        // TODO: Add batch inserts? There should not be that many publisher to be a performance problem.
        
        DataController.perform { context in
            list.forEach {
                insertInternal(publisherID: $0.publisherID, publisherLogo: $0.publisherLogo,
                               publisherName: $0.publisherName, context: .existing(context))
            }
        }
    }
    
    class func insertInternal(enabled: Bool = true, publisherID: String, publisherLogo: String?,
                              publisherName: String, context: WriteContext = .new(inMemory: false)) {
        
        DataController.perform(context: context) { context in
            let source = BraveTodaySourceMO(entity: entity(in: context), insertInto: context)
            
            source.enabled = enabled
            source.publisherID = publisherID
            source.publisherLogo = publisherLogo
        }
    }
    
    private class func entity(in context: NSManagedObjectContext) -> NSEntityDescription {
        NSEntityDescription.entity(forEntityName: "BraveTodaySourceMO", in: context)!
    }
}
