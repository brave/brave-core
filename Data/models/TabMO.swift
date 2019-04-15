/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import CoreData
import Foundation
import FastImageCache
import Shared
import WebKit
import XCGLogger

/// Properties we want to extract from Tab/TabManager and save in TabMO
public struct SavedTab {
    public let id: String
    public let title: String?
    public let url: String
    public let isSelected: Bool
    public let order: Int16
    public let screenshot: UIImage?
    public let history: [String]
    public let historyIndex: Int16
    
    public init(id: String, title: String?, url: String, isSelected: Bool, order: Int16, screenshot: UIImage?, 
                history: [String], historyIndex: Int16) {
        self.id = id
        self.title = title
        self.url = url
        self.isSelected = isSelected
        self.order = order
        self.screenshot = screenshot
        self.history = history
        self.historyIndex = historyIndex
    }
}

private let log = Logger.browserLogger

public final class TabMO: NSManagedObject, CRUD {
    
    @NSManaged public var title: String?
    @NSManaged public var url: String?
    @NSManaged public var syncUUID: String?
    @NSManaged public var order: Int16
    @NSManaged public var urlHistorySnapshot: NSArray? // array of strings for urls
    @NSManaged public var urlHistoryCurrentIndex: Int16
    @NSManaged public var screenshot: Data?
    @NSManaged public var isSelected: Bool
    @NSManaged public var color: String?
    @NSManaged public var screenshotUUID: String?
    
    public override func prepareForDeletion() {
        super.prepareForDeletion()
        
        // BRAVE TODO: check, if we still need it for restoring website screenshots.
        // Remove cached image
        //        if let url = imageUrl, !PrivateBrowsing.singleton.isOn {
        //            ImageCache.shared.remove(url, type: .portrait)
        //        }
    }
    
    // MARK: - Public interface
    
    // MARK: Create
    
    /// Creates new tab and returns its syncUUID. If you want to add urls to existing tabs use `update()` method.
    public class func create(uuidString: String = UUID().uuidString) -> String {
        
        DataController.perform(task: { context in
            let tab = TabMO(entity: entity(context), insertInto: context)
            // TODO: replace with logic to create sync uuid then buble up new uuid to browser.
            tab.syncUUID = uuidString
            tab.title = Strings.New_Tab
        })
        
        return uuidString
    }
    
    // MARK: Read
    
    public class func getAll() -> [TabMO] {
        let sortDescriptors = [NSSortDescriptor(key: #keyPath(TabMO.order), ascending: true)]
        return all(sortDescriptors: sortDescriptors) ?? []
    }
    
    public class func get(fromId id: String?) -> TabMO? {
        return getInternal(fromId: id)
    }
    
    // MARK: Update
    
    // Updates existing tab with new data.
    // Usually called when user navigates to a new website for in his existing tab.
    public class func update(tabData: SavedTab) {
        DataController.perform { context in
            guard let tabToUpdate = getInternal(fromId: tabData.id, context: context) else { return }
            
            if let screenshot = tabData.screenshot {
                tabToUpdate.screenshot = screenshot.jpegData(compressionQuality: 1)
            }
            tabToUpdate.url = tabData.url
            tabToUpdate.order = tabData.order
            tabToUpdate.title = tabData.title
            tabToUpdate.urlHistorySnapshot = tabData.history as NSArray
            tabToUpdate.urlHistoryCurrentIndex = tabData.historyIndex
            tabToUpdate.isSelected = tabData.isSelected
        }
    }
    
    public class func saveScreenshotUUID(_ uuid: UUID?, tabId: String?) {
        DataController.perform { context in
            let tabMO = getInternal(fromId: tabId, context: context)
            tabMO?.screenshotUUID = uuid?.uuidString
        }
    }
    
    public class func saveTabOrder(tabIds: [String]) {
        DataController.perform { context in
            for (i, tabId) in tabIds.enumerated() {
                guard let managedObject = getInternal(fromId: tabId, context: context) else {
                    log.error("Error: Tab missing managed object")
                    continue
                }
                managedObject.order = Int16(i)
            }
        }
    }
    
    // MARK: Delete
    
    public func delete() {
        delete(context: .new)
    }
    
    public class func deleteAll() {
        deleteAll(context: .new)
    }
    
    public class func deleteAllPrivateTabs() {
        deleteAll(predicate: NSPredicate(format: "isPrivate == true"), context: .new)
    }

    }

// MARK: - Internal implementations
extension TabMO {
    // Currently required, because not `syncable`
    private static func entity(_ context: NSManagedObjectContext) -> NSEntityDescription {
        return NSEntityDescription.entity(forEntityName: "TabMO", in: context)!
    }
    
    private class func getInternal(fromId id: String?,
                                   context: NSManagedObjectContext = DataController.viewContext) -> TabMO? {
        guard let id = id else { return nil }
        let predicate = NSPredicate(format: "\(#keyPath(TabMO.syncUUID)) == %@", id)
        
        return first(where: predicate, context: context)
    }

    var imageUrl: URL? {
        if let objectId = self.syncUUID, let url = URL(string: "https://imagecache.mo/\(objectId).png") {
            return url
        }
        return nil
    }
}
