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
    
    public static func frc() -> NSFetchedResultsController<Device> {
        let context = DataController.viewContext
        let fetchRequest = NSFetchRequest<Device>()
        fetchRequest.entity = Device.entity(context: context)
        
        let currentDeviceSort = NSSortDescriptor(key: "isCurrentDevice", ascending: false)
        fetchRequest.sortDescriptors = [currentDeviceSort]
        
        return NSFetchedResultsController(fetchRequest: fetchRequest, managedObjectContext: context,
                                          sectionNameKeyPath: nil, cacheName: nil)
    }
    
    public static func add(rootObject root: SyncRecord?, save: Bool, sendToSync: Bool, context: NSManagedObjectContext) -> Syncable? {
        
        // No guard, let bleed through to allow 'empty' devices (e.g. local)
        let root = root as? SyncDevice
        let device = Device(entity: Device.entity(context: context), insertInto: context)
        
        device.created = root?.syncNativeTimestamp ?? Date()
        device.syncUUID = root?.objectId ?? SyncCrypto.uniqueSerialBytes(count: 16)
        
        device.update(syncRecord: root)
        
        if save {
            DataController.save(context: context)
        }
        
        return device
    }
    
    public class func add(name: String?, isCurrent: Bool = false) {
        let context = DataController.newBackgroundContext()
        
        let device = Device(entity: Device.entity(context: context), insertInto: context)
        device.created = Date()
        device.syncUUID = SyncCrypto.uniqueSerialBytes(count: 16)
        device.name = name
        device.isCurrentDevice = isCurrent
        
        DataController.save(context: context)
    }
    
    public func update(syncRecord record: SyncRecord?) {
        guard let root = record as? SyncDevice else { return }
        self.name = root.name
        self.deviceId = root.deviceId
        
        // No save currently
    }
    
    /// Returns a current device and assings it to a shared variable.
    public static func currentDevice() -> Device? {
        if sharedCurrentDevice == nil {
            let predicate = NSPredicate(format: "isCurrentDevice == true")
            sharedCurrentDevice = first(where: predicate)
        }
        
        return sharedCurrentDevice
    }
    
    public class func deleteAll() {
        sharedCurrentDevice = nil
        Device.deleteAll(includesPropertyValues: false)
    }
    
    // BRAVE TODO:
    /*
    public class func deviceSettings(profile: Profile) -> [SyncDeviceSetting]? {
        // Building settings off of device objects
        
        let deviceSettings = Device.all()?.map {
            // Even if no 'real' title, still want it to show up in list
            return SyncDeviceSetting(profile: profile, device: $0)
        }
        
        return deviceSettings
    }
    */
}
