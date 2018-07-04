/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import CoreData
import Foundation
import FastImageCache
import Shared
import WebKit
import XCGLogger

// Properties we want to extract from Tab/TabManager and save in TabMO
public typealias SavedTab = (id: String, title: String, url: String, isSelected: Bool, order: Int16, screenshot: UIImage?, history: [String], historyIndex: Int16)

private let log = Logger.browserLogger

public class TabMO: NSManagedObject {
    
    @NSManaged public var title: String?
    @NSManaged public var url: String?
    @NSManaged public var syncUUID: String?
    @NSManaged public var order: Int16
    @NSManaged public var urlHistorySnapshot: NSArray? // array of strings for urls
    @NSManaged public var urlHistoryCurrentIndex: Int16
    @NSManaged public var screenshot: Data?
    @NSManaged public var isSelected: Bool
    @NSManaged public var color: String?
    @NSManaged public var screenshotUUID: UUID?
    
    public var imageUrl: URL? {
        if let objectId = self.syncUUID, let url = URL(string: "https://imagecache.mo/\(objectId).png") {
            return url
        }
        return nil
    }
    
    public override func prepareForDeletion() {
        super.prepareForDeletion()

        // BRAVE TODO: uncomment
        // Remove cached image
//        if let url = imageUrl, !PrivateBrowsing.singleton.isOn {
//            ImageCache.shared.remove(url, type: .portrait)
//        }
    }

    // Currently required, because not `syncable`
    static func entity(_ context: NSManagedObjectContext) -> NSEntityDescription {
        return NSEntityDescription.entity(forEntityName: "TabMO", in: context)!
    }
    
    /// Creates new tab. If you want to add urls to existing tabs use `update()` method. 
    public class func create(_ context: NSManagedObjectContext = DataController.shared.mainThreadContext) -> TabMO {
        let tab = TabMO(entity: TabMO.entity(context), insertInto: context)
        // TODO: replace with logic to create sync uuid then buble up new uuid to browser.
        tab.syncUUID = UUID().uuidString
        tab.title = Strings.New_Tab
        DataController.saveContext(context: context)
        return tab
    }

    // Updates existing tab with new data. Usually called when user navigates to a new website for in his existing tab.
    @discardableResult 
    public class func update(with id: String, tabData: SavedTab, context: NSManagedObjectContext) -> TabMO? {
        guard let tab = get(by: id, context: context) else { return nil }
        
        if let s = tabData.screenshot {
            tab.screenshot = UIImageJPEGRepresentation(s, 1)
        }
        tab.url = tabData.url
        tab.order = tabData.order
        tab.title = tabData.title
        tab.urlHistorySnapshot = tabData.history as NSArray
        tab.urlHistoryCurrentIndex = tabData.historyIndex
        tab.isSelected = tabData.isSelected
        
        DataController.saveContext(context: context)
        
        return tab
    }
    
    public class func preserve(savedTab: SavedTab) {
        let context = DataController.shared.workerContext
        context.perform {
            TabMO.update(with: savedTab.id, tabData: savedTab, context: context)
        }
    }

    public class func getAll() -> [TabMO] {
        let fetchRequest = NSFetchRequest<NSFetchRequestResult>()
        let context = DataController.shared.mainThreadContext
        
        fetchRequest.entity = TabMO.entity(context)
        fetchRequest.sortDescriptors = [NSSortDescriptor(key: #keyPath(TabMO.order), ascending: true)]
        do {
            return try context.fetch(fetchRequest) as? [TabMO] ?? []
        } catch {
            let fetchError = error as NSError
            print(fetchError)
        }
        return []
    }
    
    public class func clearAllPrivate() {
        let fetchRequest = NSFetchRequest<NSFetchRequestResult>()
        let context = DataController.shared.mainThreadContext
        
        fetchRequest.entity = TabMO.entity(context)
        fetchRequest.sortDescriptors = [NSSortDescriptor(key: #keyPath(TabMO.order), ascending: true)]
        do {
            let results = try context.fetch(fetchRequest) as? [TabMO] ?? []
            for tab in results {
                DataController.remove(object: tab)
            }
        } catch {
            let fetchError = error as NSError
            print(fetchError)
        }
    }
    
    public class func get(by id: String?, context: NSManagedObjectContext) -> TabMO? {
        guard let id = id else { return nil }
        
        let fetchRequest = NSFetchRequest<NSFetchRequestResult>()
        fetchRequest.entity = TabMO.entity(context)
        fetchRequest.predicate = NSPredicate(format: "\(#keyPath(TabMO.syncUUID)) == %@", id)
        var result: TabMO? = nil
        do {
            let results = try context.fetch(fetchRequest) as? [TabMO]
            if let item = results?.first {
                result = item
            }
        } catch {
            let fetchError = error as NSError
            print(fetchError)
        }
        return result
    }
}

