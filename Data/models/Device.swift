/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */


import UIKit
import CoreData
import Foundation
import Shared

public final class Device: NSManagedObject, Syncable, CRUD {
    
    // Check if this can be nested inside the method
    static var sharedCurrentDevice: Device?
    
    // Assign on parent model via CD
    @NSManaged public var isSynced: Bool
    
    @NSManaged public var created: Date?
    @NSManaged public var isCurrentDevice: Bool
    @NSManaged public var deviceDisplayId: String?
    @NSManaged public var syncDisplayUUID: String?
    @NSManaged public var name: String?
    
    // Device is subtype of prefs 🤢
    public var recordType: SyncRecordType = .prefs

    // Just a facade around the displayId, for easier access and better CD storage
    var deviceId: [Int]? {
        get { return SyncHelpers.syncUUID(fromString: deviceDisplayId) }
        set(value) { deviceDisplayId = SyncHelpers.syncDisplay(fromUUID: value) }
    }
    
    // This should be abstractable
    public func asDictionary(deviceId: [Int]?, action: Int?) -> [String: Any] {
        return SyncDevice(record: self, deviceId: deviceId, action: action).dictionaryRepresentation()
    }
    
    public static func add(rootObject root: SyncRecord?, save: Bool, sendToSync: Bool, context: NSManagedObjectContext) -> Syncable? {
        
        // No guard, let bleed through to allow 'empty' devices (e.g. local)
        let root = root as? SyncDevice
        let device = Device(entity: Device.entity(context: context), insertInto: context)
        
        context.perform {
            device.created = root?.syncNativeTimestamp ?? Date()
            device.syncUUID = root?.objectId ?? SyncCrypto.uniqueSerialBytes(count: 16)
            
            device.update(syncRecord: root)
            
            if save {
                DataController.save(context: context)
            }
        }
        
        return device
    }
    
    public class func add(save: Bool = false, context: NSManagedObjectContext) -> Device? {
        return add(rootObject: nil, save: save, sendToSync: false, context: context) as? Device
    }
    
    public func update(syncRecord record: SyncRecord?) {
        guard let root = record as? SyncDevice else { return }
        self.name = root.name
        self.deviceId = root.deviceId
        
        // No save currently
    }
    
    public static func currentDevice() -> Device? {
        
        if sharedCurrentDevice == nil {
            let context = DataController.newBackgroundContext()
            // Create device
            let predicate = NSPredicate(format: "isCurrentDevice = YES")
            // Should only ever be one current device!
            var localDevice: Device? = get(predicate: predicate, context: context)?.first
            
            if localDevice == nil {
                // Create
                localDevice = add(context: context)
                localDevice?.isCurrentDevice = true
                DataController.save(context: context)
            }
            
            sharedCurrentDevice = localDevice
        }
        return sharedCurrentDevice
    }
    
    public class func deleteAll() {
        let context = DataController.newBackgroundContext()
        context.perform {
            let fetchRequest = NSFetchRequest<NSFetchRequestResult>()
            fetchRequest.entity = Device.entity(context: context)
            fetchRequest.includesPropertyValues = false
            do {
                let results = try context.fetch(fetchRequest)
                for result in results {
                    context.delete(result as! NSManagedObject)
                }
                
            } catch {
                let fetchError = error as NSError
                print(fetchError)
            }

            // Destroy handle to local device instance, otherwise it is locally retained and will throw console errors
            sharedCurrentDevice = nil
            
            DataController.save(context: context)
        }
    }
}
