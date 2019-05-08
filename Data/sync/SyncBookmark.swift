/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import SwiftyJSON

final class SyncBookmark: SyncRecord {
    
    // MARK: Declaration for string constants to be used to decode and also serialize.
    fileprivate struct SerializationKeys {
        static let isFolder = "isFolder"
        static let parentFolderObjectId = "parentFolderObjectId"
        static let site = "site"
        static let syncOrder = "order"
    }
    
    // MARK: Properties
    var isFavorite: Bool = false
    var isFolder: Bool? = false
    var parentFolderObjectId: [Int]?
    var site: SyncSite?
    var syncOrder: String?
    
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
    
    required init(record: Syncable?, deviceId: [Int]?, action: Int?) {
        super.init(record: record, deviceId: deviceId, action: action)
        
        let bm = record as? Bookmark
        
        let unixCreated = Int(bm?.created?.toTimestamp() ?? 0)
        let unixAccessed = Int(bm?.lastVisited?.toTimestamp() ?? 0)
        
        let site = SyncSite()
        site.title = bm?.title
        site.customTitle = bm?.customTitle
        site.location = bm?.url
        site.creationTime = unixCreated
        site.lastAccessedTime = unixAccessed
        // FIXME: This sometimes crashes the app. See issue #1760.
        // site.favicon = bm?.domain?.favicon?.url
        
        self.isFavorite = bm?.isFavorite ?? false
        self.isFolder = bm?.isFolder
        self.parentFolderObjectId = bm?.syncParentUUID
        self.site = site
        syncOrder = bm?.syncOrder
    }
    
    /// Initiates the instance based on the JSON that was passed.
    ///
    /// - parameter json: JSON object from SwiftyJSON.
    required init(json: JSON?) {
        super.init(json: json)
        
        guard let objectData = self.objectData else { return }
        
        let bookmark = json?[objectData.rawValue]
        isFolder = bookmark?[SerializationKeys.isFolder].bool
        syncOrder = bookmark?[SerializationKeys.syncOrder].string
        if let items = bookmark?[SerializationKeys.parentFolderObjectId].array { parentFolderObjectId = items.map { $0.intValue } }
        site = SyncSite(json: bookmark?[SerializationKeys.site])
    }
    
    /// Generates description of the object in the form of a NSDictionary.
    ///
    /// - returns: A Key value pair containing all valid values in the object.
    override func dictionaryRepresentation() -> [String: Any] {
        guard let objectData = self.objectData else { return [:] }
        
        // Create nested bookmark dictionary
        var bookmarkDict = [String: Any]()
        bookmarkDict[SerializationKeys.isFolder] = isFolder
        bookmarkDict[SerializationKeys.syncOrder] = syncOrder
        if let value = parentFolderObjectId { bookmarkDict[SerializationKeys.parentFolderObjectId] = value }
        if let value = site { bookmarkDict[SerializationKeys.site] = value.dictionaryRepresentation() }
        
        // Fetch parent, and assign bookmark
        var dictionary = super.dictionaryRepresentation()
        dictionary[objectData.rawValue] = bookmarkDict
        
        return dictionary
    }
    
}
