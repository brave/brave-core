/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import CoreData
import SwiftyJSON

private let log = Logger.braveSyncLogger

protocol Syncable: class /* where Self: NSManagedObject */ {
    // Used to enforce CD conformity
    /* @NSManaged */ var syncDisplayUUID: String? { get set }
    /* @NSManaged */ var created: Date? { get set }
    
    // Primarily used for generic record deletion
    var recordType: SyncRecordType { get }
    
    static func entity(context: NSManagedObjectContext) -> NSEntityDescription
    
    func asDictionary(deviceId: [Int]?, action: Int?) -> [String: Any]
    
    // The most important difference between these methods and regular CRUD operations is that
    // resolved records from sync should be never sent back to the sync server.
    static func createResolvedRecord(rootObject root: SyncRecord?, save: Bool, context: WriteContext)
    func updateResolvedRecord(_ record: SyncRecord?, context: WriteContext)
    func deleteResolvedRecord(save: Bool, context: NSManagedObjectContext)
}

extension Syncable {
    static func entity(context: NSManagedObjectContext) -> NSEntityDescription {
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
        
        let searchableUUIDs = syncUUIDs.compactMap { SyncHelpers.syncDisplay(fromUUID: $0) }
        return get(predicate: NSPredicate(format: "syncDisplayUUID IN %@", searchableUUIDs), context: context)
    }
    
    static func get(predicate: NSPredicate?, context: NSManagedObjectContext) -> [NSManagedObject]? {
        let fetchRequest = NSFetchRequest<NSFetchRequestResult>()
        
        fetchRequest.entity = Self.entity(context: context)
        fetchRequest.predicate = predicate
        
        var result: [NSManagedObject]?
        context.performAndWait {
            
            do {
                result = try context.fetch(fetchRequest) as? [NSManagedObject]
            } catch {
                log.error(error)
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
            log.error(error)
        }
        
        return nil
    }
}

class SyncHelpers {
    // Converters
    
    /// UUID -> DisplayUUID
    static func syncDisplay(fromUUID uuid: [Int]?) -> String? {
        return uuid?.map { $0.description }.joined(separator: ",")
    }
    
    /// DisplayUUID -> UUID
    static func syncUUID(fromString string: String?) -> [Int]? {
        return string?.components(separatedBy: ",").compactMap { Int($0) }
    }
    
    static func syncUUID(fromJSON json: JSON?) -> [Int]? {
        return json?.array?.compactMap { $0.int }
    }
}
