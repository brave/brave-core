/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import CoreData
import SwiftyJSON

protocol Syncable: class /* where Self: NSManagedObject */ {
    // Used to enforce CD conformity
    /* @NSManaged */ var syncDisplayUUID: String? { get set }
    /* @NSManaged */ var created: Date? { get set }
    
    // Primarily used for generic record deletion
    var recordType: SyncRecordType { get }
    
    static func entity(context:NSManagedObjectContext) -> NSEntityDescription
    
    func asDictionary(deviceId: [Int]?, action: Int?) -> [String: Any]
    
    func update(syncRecord record: SyncRecord?)
    
    @discardableResult static func add(rootObject root: SyncRecord?, save: Bool, sendToSync: Bool, context: NSManagedObjectContext) -> Syncable?
}

extension Syncable {
    static func entity(context:NSManagedObjectContext) -> NSEntityDescription {
        // Swift 4 version
        // let className = String(describing: type(of: self))
        let className = String(describing: self)
        return NSEntityDescription.entity(forEntityName: className, in: context)!
    }
    
    static func get(syncUUIDs: [[Int]]?, context: NSManagedObjectContext) -> [NSManagedObject]? {
        
        guard let syncUUIDs = syncUUIDs else {
            return nil
        }
        
        // TODO: filter a unique set of syncUUIDs
        
        let searchableUUIDs = syncUUIDs.map { SyncHelpers.syncDisplay(fromUUID: $0) }.flatMap { $0 }
        return get(predicate: NSPredicate(format: "syncDisplayUUID IN %@", searchableUUIDs), context: context)
    }
    
    static func get(predicate: NSPredicate?, context: NSManagedObjectContext) -> [NSManagedObject]? {
        let fetchRequest = NSFetchRequest<NSFetchRequestResult>()
        
        fetchRequest.entity = Self.entity(context: context)
        fetchRequest.predicate = predicate
        
        var result: [NSManagedObject]? = nil
        context.performAndWait {
            
            
            do {
                result = try context.fetch(fetchRequest) as? [NSManagedObject]
            } catch {
                let fetchError = error as NSError
                print(fetchError)
            }
        }
        
        return result
    }
}

//extension Syncable where Self: NSManagedObject {
extension Syncable {
    
    // Is conveted to better store in CD
    var syncUUID: [Int]? {
        get { return SyncHelpers.syncUUID(fromString: syncDisplayUUID) }
        set(value) { syncDisplayUUID = SyncHelpers.syncDisplay(fromUUID: value) }
    }
    
    // Maybe use 'self'?
    static func get<T: Syncable>(predicate: NSPredicate?, context: NSManagedObjectContext?) -> [T]? {
        guard let context = context else {
            // error
            return nil
        }
        let fetchRequest = NSFetchRequest<NSFetchRequestResult>()
        
        fetchRequest.entity = T.entity(context: context)
        fetchRequest.predicate = predicate
        
        do {
            return try context.fetch(fetchRequest) as? [T]
        } catch {
            let fetchError = error as NSError
            print(fetchError)
        }
        
        return nil
    }
}

extension Syncable /* where Self: NSManagedObject */ {
    func remove(save: Bool) {
        
        // This is r annoying, and can be fixed in Swift 4, but since objects can't be cast to a class & protocol,
        //  but given extension on Syncable, if this passes the object is both Syncable and an NSManagedObject subclass
        guard let s = self as? NSManagedObject, let context = s.managedObjectContext else { return }
        
        // Must happen before, otherwise bookmark is gone
        
        Sync.shared.sendSyncRecords(action: .delete, records: [self])
        
        // Should actually delay, and wait for server to refetch records to confirm deletion.
        // Force a sync resync instead, should not be slow
        context.delete(s)
        if save {
            DataController.saveContext(context: context)
        }
    }
}

class SyncHelpers {
    // Converters
    
    /// UUID -> DisplayUUID
    static func syncDisplay(fromUUID uuid: [Int]?) -> String? {
        return uuid?.map{ $0.description }.joined(separator: ",")
    }
    
    /// DisplayUUID -> UUID
    static func syncUUID(fromString string: String?) -> [Int]? {
        return string?.components(separatedBy: ",").flatMap { Int($0) }
    }
    
    static func syncUUID(fromJSON json: JSON?) -> [Int]? {
        return json?.array?.flatMap { $0.int }
    }
}
