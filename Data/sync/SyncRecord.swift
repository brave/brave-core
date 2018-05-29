/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import CoreData
import SwiftyJSON

protocol SyncRecordProtocol {
    associatedtype CoreDataParallel: Syncable
//    var CoredataParallel: NSManagedObject.Type?
    
}

class SyncRecord: SyncRecordProtocol {
    
    // MARK: Declaration for string constants to be used to decode and also serialize.
    fileprivate struct SerializationKeys {
        static let objectId = "objectId"
        static let deviceId = "deviceId"
        static let action = "action"
        static let objectData = "objectData"
        // TODO: Add sync timestamp
    }
    
    // MARK: Properties
    var objectId: [Int]?
    var deviceId: [Int]?
    var action: Int?
    var objectData: SyncObjectDataType?
    
//    var CoredataParallel: Syncable.Type?
    typealias CoreDataParallel = Device
    
    convenience init() {
        self.init(json: nil)
    }
    
    /// Initiates the instance based on the object.
    ///
    /// - parameter object: The object of either Dictionary or Array kind that was passed.
    /// - returns: An initialized instance of the class.
    convenience init(object: [String: AnyObject]) {
        self.init(json: JSON(object))
    }
    
    // Would be nice to make this type specific to class
    required init(record: Syncable?, deviceId: [Int]?, action: Int?) {
        
        self.objectId = record?.syncUUID
        self.deviceId = deviceId
        self.action = action
        
        // TODO: Move to SyncObjectDataType enum
//        self.objectData = [Syncable.Type: SyncObjectDataType] = [Bookmark.self: .Bookmark][self.Type]
        self.objectData = .Bookmark
        
        // TODO: Need object type!!
        
        // TOOD: Add sync timestmap
    }
    
    /// Initiates the instance based on the JSON that was passed.
    ///
    /// - parameter json: JSON object from SwiftyJSON.
    required init(json: JSON?) {
        // objectId can come in two different formats
        if let items = json?[SerializationKeys.objectId].array { objectId = items.map { $0.intValue } }
        if let items = json?[SerializationKeys.deviceId].array { deviceId = items.map { $0.intValue } }
        action = json?[SerializationKeys.action].int
        if let item = json?[SerializationKeys.objectData].string { objectData = SyncObjectDataType(rawValue: item) }
        // TODO: Add sync timestamp
    }
    
    /// Generates description of the object in the form of a NSDictionary.
    ///
    /// - returns: A Key value pair containing all valid values in the object.
    func dictionaryRepresentation() -> [String: Any] {
        var dictionary: [String: Any] = [:]
        // Override to use string value instead of array, to be uniform to CD
        if let value = objectId { dictionary[SerializationKeys.objectId] = value }
        if let value = deviceId { dictionary[SerializationKeys.deviceId] = value }
        if let value = action { dictionary[SerializationKeys.action] = value }
        if let value = objectData { dictionary[SerializationKeys.objectData] = value.rawValue }
        // TODO: Add sync timestamp
        return dictionary
    }
}

// Uses same mappings above, but for arrays
extension SyncRecordProtocol where Self: SyncRecord {
    
    static func syncRecords(_ rootJSON: [JSON]?) -> [Self]? {
        return rootJSON?.map {
            return self.init(json: $0)
        }
    }
    
    static func syncRecords(_ rootJSON: JSON) -> [Self]? {
        return self.syncRecords(rootJSON.array)
    }
}


