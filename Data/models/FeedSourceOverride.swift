// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import CoreData

public final class FeedSourceOverride: NSManagedObject, CRUD {
    @NSManaged public var enabled: Bool
    @NSManaged public var publisherID: String
    
    public class func get(fromId id: String) -> FeedSourceOverride? {
        getInternal(fromId: id)
    }
    
    public class func all() -> [FeedSourceOverride] {
        all() ?? []
    }
    
    public class func setEnabled(forId id: String, enabled: Bool) {
        setEnabledInternal(forId: id, enabled: enabled)
    }
    
    public class func setEnabled(forIds ids: [String], enabled: Bool) {
        DataController.perform(context: .new(inMemory: false)) { context in
            ids.forEach {
                setEnabledInternal(forId: $0, enabled: enabled, context: .existing(context))
            }
        }
    }
    
    public class func resetSourceSelection() {
        deleteAll()
    }
    
    class func getInternal(fromId id: String, context: NSManagedObjectContext = DataController.viewContext) -> FeedSourceOverride? {
        let predicate = NSPredicate(format: "\(#keyPath(FeedSourceOverride.publisherID)) == %@", id)
        return first(where: predicate, context: context)
    }
    
    class func setEnabledInternal(forId id: String, enabled: Bool, context: WriteContext = .new(inMemory: false)) {
        DataController.perform(context: context) { context in
            if let source = getInternal(fromId: id, context: context) {
                source.enabled = enabled
            } else {
                insertInternal(publisherID: id, enabled: enabled, context: .existing(context))
            }
        }
    }
    
    class func insertInternal(publisherID: String, enabled: Bool, context: WriteContext = .new(inMemory: false)) {
        DataController.perform(context: context) { context in
            let source = FeedSourceOverride(entity: entity(in: context), insertInto: context)
            
            source.enabled = enabled
            source.publisherID = publisherID
        }
    }
    
    private class func entity(in context: NSManagedObjectContext) -> NSEntityDescription {
        NSEntityDescription.entity(forEntityName: "FeedSourceOverride", in: context)!
    }
}
