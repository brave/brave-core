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

    public override func awakeFromInsert() {
        super.awakeFromInsert()
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
    
    public class func freshTab(_ context: NSManagedObjectContext = DataController.shared.mainThreadContext) -> TabMO {
        let tab = TabMO(entity: TabMO.entity(context), insertInto: context)
        // TODO: replace with logic to create sync uuid then buble up new uuid to browser.
        tab.syncUUID = UUID().uuidString
        tab.title = Strings.New_Tab
        DataController.saveContext(context: context)
        return tab
    }

    @discardableResult 
    public class func add(_ tabInfo: SavedTab, context: NSManagedObjectContext) -> TabMO? {
        guard let tab = get(byId: tabInfo.id, context: context) else { return nil }
        
        if let s = tabInfo.screenshot {
            tab.screenshot = UIImageJPEGRepresentation(s, 1)
        }
        tab.url = tabInfo.url
        tab.order = tabInfo.order
        tab.title = tabInfo.title
        tab.urlHistorySnapshot = tabInfo.history as NSArray
        tab.urlHistoryCurrentIndex = tabInfo.historyIndex
        tab.isSelected = tabInfo.isSelected
        return tab
    }

    public class func getAll() -> [TabMO] {
        let fetchRequest = NSFetchRequest<NSFetchRequestResult>()
        let context = DataController.shared.mainThreadContext
        
        fetchRequest.entity = TabMO.entity(context)
        fetchRequest.predicate = NSPredicate(format: "isPrivate == false OR isPrivate == nil")
        fetchRequest.sortDescriptors = [NSSortDescriptor(key: "order", ascending: true)]
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
        fetchRequest.predicate = NSPredicate(format: "isPrivate == true")
        fetchRequest.sortDescriptors = [NSSortDescriptor(key: "order", ascending: true)]
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
    
    public class func get(byId id: String?, context: NSManagedObjectContext) -> TabMO? {
        guard let id = id else { return nil }
        
        let fetchRequest = NSFetchRequest<NSFetchRequestResult>()
        fetchRequest.entity = TabMO.entity(context)
        fetchRequest.predicate = NSPredicate(format: "syncUUID == %@", id)
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
    
    public class func preserve(savedTab: SavedTab, urlOverride: String? = nil) {
        let context = DataController.shared.workerContext
        context.perform {
            _ = TabMO.add(savedTab, context: context)
            DataController.saveContext(context: context)
        }
    }
}

