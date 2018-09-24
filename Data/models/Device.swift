/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import CoreData
import Foundation
import Shared

class Device: NSManagedObject, Syncable {
    
    // Check if this can be nested inside the method
    static var sharedCurrentDevice: Device?
    
    // Assign on parent model via CD
    @NSManaged var isSynced: Bool
    
    @NSManaged var created: Date?
    @NSManaged var isCurrentDevice: Bool
    @NSManaged var deviceDisplayId: String?
    @NSManaged var syncDisplayUUID: String?
    @NSManaged var name: String?
    
    // Device is subtype of prefs 🤢
    var recordType: SyncRecordType = .prefs

    // Just a facade around the displayId, for easier access and better CD storage
    var deviceId: [Int]? {
        get { return SyncHelpers.syncUUID(fromString: deviceDisplayId) }
        set(value) { deviceDisplayId = SyncHelpers.syncDisplay(fromUUID: value) }
    }
    
    // This should be abstractable
    func asDictionary(deviceId: [Int]?, action: Int?) -> [String: Any] {
        return SyncDevice(record: self, deviceId: deviceId, action: action).dictionaryRepresentation()
    }
    
    @discardableResult static func add(rootObject root: SyncRecord?, save: Bool, sendToSync: Bool, context: NSManagedObjectContext) -> Syncable? {
        
        // No guard, let bleed through to allow 'empty' devices (e.g. local)
        let root = root as? SyncDevice

        let device = Device(entity: Device.entity(context: context), insertInto: context)
        
        device.created = root?.syncNativeTimestamp ?? Date()
        device.syncUUID = root?.objectId ?? SyncCrypto.shared.uniqueSerialBytes(count: 16)

        device.update(syncRecord: root)
        
        if save {
            DataController.saveContext(context: context)
        }
        
        return device
    }
    
    class func add(save: Bool = false, context: NSManagedObjectContext) -> Device? {
        return add(rootObject: nil, save: save, sendToSync: false, context: context) as? Device
    }
    
    func update(syncRecord record: SyncRecord?) {
        guard let root = record as? SyncDevice else { return }
        self.name = root.name
        self.deviceId = root.deviceId
        
        // No save currently
    }
    
    static func currentDevice() -> Device? {
        
        if sharedCurrentDevice == nil {
            let context = DataController.workerThreadContext
            // Create device
            let predicate = NSPredicate(format: "isCurrentDevice = YES")
            // Should only ever be one current device!
            var localDevice: Device? = get(predicate: predicate, context: context)?.first
            
            if localDevice == nil {
                // Create
                localDevice = add(context: context)
                localDevice?.isCurrentDevice = true
                DataController.saveContext(context: context)
            }
            
            sharedCurrentDevice = localDevice
        }
        return sharedCurrentDevice
    }
    
    class func deleteAll(completionOnMain: () -> Void) {
        let context = DataController.workerThreadContext
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
            
            DataController.saveContext(context: context)
        }
    }
}
