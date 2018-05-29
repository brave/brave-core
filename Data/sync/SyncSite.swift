/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import SwiftyJSON

public final class SyncSite {
    
    // MARK: Declaration for string constants to be used to decode and also serialize.
    fileprivate struct SerializationKeys {
        static let customTitle = "customTitle"
        static let title = "title"
        static let favicon = "favicon"
        static let location = "location"
        static let creationTime = "creationTime"
        static let lastAccessedTime = "lastAccessedTime"
    }
    
    // MARK: Properties
    public var customTitle: String?
    public var title: String?
    public var favicon: String?
    public var location: String?
    public var creationTime: Int?
    public var lastAccessedTime: Int?
    
    public var creationNativeDate: Date? {
        return Date.fromTimestamp(Timestamp(creationTime ?? 0))
    }
    
    public var lastAccessedNativeDate: Date? {
        return Date.fromTimestamp(Timestamp(lastAccessedTime ?? 0))
    }
    
    public convenience init() {
        self.init(json: nil)
    }
    
    /// Initiates the instance based on the object.
    ///
    /// - parameter object: The object of either Dictionary or Array kind that was passed.
    /// - returns: An initialized instance of the class.
    public convenience init(object: [String: AnyObject]) {
        self.init(json: JSON(object))
    }
    
    /// Initiates the instance based on the JSON that was passed.
    ///
    /// - parameter json: JSON object from SwiftyJSON.
    public required init(json: JSON?) {
        customTitle = json?[SerializationKeys.customTitle].string
        title = json?[SerializationKeys.title].string
        favicon = json?[SerializationKeys.favicon].string
        location = json?[SerializationKeys.location].string
        creationTime = json?[SerializationKeys.creationTime].int
        lastAccessedTime = json?[SerializationKeys.lastAccessedTime].int
    }
    
    /// Generates description of the object in the form of a NSDictionary.
    ///
    /// - returns: A Key value pair containing all valid values in the object.
    public func dictionaryRepresentation() -> [String: AnyObject] {
        var dictionary: [String: AnyObject] = [:]
        if let value = customTitle { dictionary[SerializationKeys.customTitle] = value as AnyObject }
        if let value = title { dictionary[SerializationKeys.title] = value as AnyObject }
        if let value = favicon { dictionary[SerializationKeys.favicon] = value as AnyObject }
        if let value = location { dictionary[SerializationKeys.location] = value as AnyObject }
        if let value = creationTime { dictionary[SerializationKeys.creationTime] = value as AnyObject }
        if let value = lastAccessedTime { dictionary[SerializationKeys.lastAccessedTime] = value as AnyObject }
        return dictionary
    }
    
}
