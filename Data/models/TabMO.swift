/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import CoreData
import Foundation
import FastImageCache
import Shared
import WebKit
import XCGLogger

public typealias SavedTab2 = (id: String, title: String, url: String, isSelected: Bool, order: Int16, screenshot: UIImage?, history: [String], historyIndex: Int16)

public typealias TabDataToSave = (webView: WKWebView?, url: URL?, order: UInt, tabID: String, displayTitle: String, 
                                  isSelected: Bool)

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
    @NSManaged public var isClosed: Bool
    @NSManaged public var isPrivate: Bool
    @NSManaged public var color: String?
    
    public var imageUrl: URL? {
        if let objectId = self.syncUUID, let url = URL(string: "https://imagecache.mo/\(objectId).png") {
            return url
        }
        return nil
    }
    
    public var sessionData: SessionData?

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
        // BRAVE TODO:
//        tab.isPrivate = PrivateBrowsing.singleton.isOn
        DataController.saveContext(context: context)
        return tab
    }

    @discardableResult 
    public class func add(_ tabInfo: SavedTab2, context: NSManagedObjectContext) -> TabMO? {
        let tab: TabMO? = get(byId: tabInfo.id, context: context)
        if tab == nil {
            return nil
        }
        if let s = tabInfo.screenshot {
            tab?.screenshot = UIImageJPEGRepresentation(s, 1)
        }
        tab?.url = tabInfo.url
        tab?.order = tabInfo.order
        tab?.title = tabInfo.title
        tab?.urlHistorySnapshot = tabInfo.history as NSArray
        tab?.urlHistoryCurrentIndex = tabInfo.historyIndex
        tab?.isSelected = tabInfo.isSelected
        return tab!
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
    
    public class func preserve(tabData: TabDataToSave, urlOverride: String? = nil) {
        
        if let data = savedTabData(tabData: tabData, urlOverride: urlOverride) {
            let context = DataController.shared.workerContext
            context.perform {
                _ = TabMO.add(data, context: context)
                DataController.saveContext(context: context)
            }
        }
    }
    
    public class func savedTabData(tabData: TabDataToSave, context: NSManagedObjectContext = DataController.shared.mainThreadContext, urlOverride: String? = nil) -> SavedTab2? {
        
        // Ignore session restore data.
        guard let urlString = tabData.url?.absoluteString else { return nil }
        if urlString.contains("localhost") { return nil }
        
        var urls = [String]()
        var currentPage = 0
        
        let webView = tabData.webView
        
        if let currentItem = webView?.backForwardList.currentItem {
            // Freshly created web views won't have any history entries at all.
            let backList = webView?.backForwardList.backList ?? []
            let forwardList = webView?.backForwardList.forwardList ?? []
            var backListMap = backList.map { $0.url.absoluteString }
            let forwardListMap = forwardList.map { $0.url.absoluteString }
            var currentItemString = currentItem.url.absoluteString
            
            log.debug("backList: \(backListMap)")
            log.debug("forwardList: \(forwardListMap)")
            log.debug("currentItem: \(currentItemString)")
            
            /* Completely ignore forward history when passing urlOverride. When a webpage
               hasn't fully loaded we attempt to preserve the current state of the webview.
               urls that are currently loading aren't visible in history or forward history.
               Our work around here is to append the currently requested URL to the end of the
               navigation stack, and ignore forward history (as it would be replaced on full load).
               There should be a very narrow edgecase where user who navigates back has no active
               cache for the back url and close the browser while page is still being loaded-
               resulting in lost forward history. */
            
            if let urlOverride = urlOverride, backListMap.count == 0 || forwardListMap.count == 0 {
                // Navigating back or forward, lets ignore current.
                if currentItemString == urlOverride {
                    currentItemString = ""
                }
                if backListMap.index(of: urlOverride) == backListMap.count - 1 {
                    backListMap.removeLast()
                }
                urls = backListMap + [currentItemString] + [urlOverride]
            }
            else {
                // Business as usual.
                urls = backListMap + [currentItemString] + forwardListMap
                currentPage = -forwardList.count
            }
            
            log.debug("---stack: \(urls)")
        }
        if let id = TabMO.get(byId: tabData.tabID, context: context)?.syncUUID {
            let displayTitle = tabData.displayTitle
            let title = displayTitle != "" ? displayTitle : urlOverride ?? ""
            if urlOverride == nil && tabData.url == nil {
                log.warning("Missing tab url, using empty string as a fallback. Should not happen.")
            }
            
            let data = SavedTab2(id, title, urlOverride ?? urlString, tabData.isSelected, Int16(tabData.order), nil, urls, Int16(currentPage))
            return data
        }
        
        return nil
    }
}

