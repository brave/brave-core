/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import CoreData
import Foundation
import BraveShared

public final class Domain: NSManagedObject, CRUD {
    
    @NSManaged public var url: String?
    @NSManaged public var visits: Int32
    @NSManaged public var topsite: Bool // not currently used. Should be used once proper frecency code is in.
    @NSManaged public var blockedFromTopSites: Bool // don't show ever on top sites
    @NSManaged public var favicon: FaviconMO?

    @NSManaged public var shield_allOff: NSNumber?
    @NSManaged public var shield_adblockAndTp: NSNumber?
    @NSManaged public var shield_httpse: NSNumber?
    @NSManaged public var shield_noScript: NSNumber?
    @NSManaged public var shield_fpProtection: NSNumber?
    @NSManaged public var shield_safeBrowsing: NSNumber?

    @NSManaged public var historyItems: NSSet?
    @NSManaged public var bookmarks: NSSet?
    
    private var urlComponents: URLComponents? {
        return URLComponents(string: url ?? "")
    }

    // Currently required, because not `syncable`
    static func entity(_ context: NSManagedObjectContext) -> NSEntityDescription {
        return NSEntityDescription.entity(forEntityName: "Domain", in: context)!
    }

    public override func awakeFromInsert() {
        super.awakeFromInsert()
    }
    
    // Returns `url` but switches the scheme from `http` <-> `https`
    private func domainForInverseHttpScheme() -> Domain? {
        
        guard var urlComponents = self.urlComponents, let context = self.managedObjectContext else {
            return nil
        }
        
        // Flip the scheme if valid
        switch urlComponents.scheme {
            case "http": urlComponents.scheme = "https"
            case "https": urlComponents.scheme = "http"
            default: return nil
        }
        
        guard let url = urlComponents.url else { return nil }
        
        // Return the flipped scheme version of `url`
        return Domain.getOrCreateForUrl(url, context: context)
    }
    
    public class func getForUrl(_ url: URL, context: NSManagedObjectContext) -> Domain? {
        let domainString = url.domainURL.absoluteString
        return Domain.first(where: NSPredicate(format: "url == %@", domainString), context: context)
    }

    public class func getOrCreateForUrl(_ url: URL, context: NSManagedObjectContext, save: Bool = true) -> Domain {
        let domainString = url.domainURL.absoluteString
        if let domain = Domain.first(where: NSPredicate(format: "url == %@", domainString), context: context) {
            return domain
        }
        
        // See #409:
        //  A larger refactor is probably wanted here.
        //  This can easily lead to a Domain being created on the `viewContext`
        //  A solution to consider is creating a new background context here, creating, saving, and then re-fetching
        //   that object in the requested context (regardless if it is `viewContext` or not)
        var newDomain: Domain!
        context.performAndWait {
            newDomain = Domain(entity: Domain.entity(context), insertInto: context)
            newDomain.url = domainString
            if save {
                DataController.save(context: context)
            }
        }
        return newDomain
    }

    class func blockFromTopSites(_ url: URL, context: NSManagedObjectContext) {
        let domain = getOrCreateForUrl(url, context: context)
        domain.blockedFromTopSites = true
        DataController.save(context: context)
    }

    class func blockedTopSites(_ context: NSManagedObjectContext) -> [Domain] {
        let blockedFromTopSitesKeyPath = #keyPath(Domain.blockedFromTopSites)
        let predicate = NSPredicate(format: "\(blockedFromTopSitesKeyPath) = YES")
        return all(where: predicate) ?? []
    }

    class func topSitesQuery(_ limit: Int, context: NSManagedObjectContext) -> [Domain] {
        let visitsKeyPath = #keyPath(Domain.visits)
        let blockedFromTopSitesKeyPath = #keyPath(Domain.blockedFromTopSites)
        let minVisits = 5
        
        let predicate = NSPredicate(format: "\(visitsKeyPath) > %i AND \(blockedFromTopSitesKeyPath) != YES", minVisits)
        let sortDescriptors = [NSSortDescriptor(key: visitsKeyPath, ascending: false)]
        
        return all(where: predicate, sortDescriptors: sortDescriptors) ?? []
    }

    class func deleteNonBookmarkedAndClearSiteVisits(context: NSManagedObjectContext, _ completionOnMain: @escaping () -> Void) {
        
        context.perform {
            let fetchRequest = NSFetchRequest<Domain>()
            fetchRequest.entity = Domain.entity(context)
            do {
                let results = try context.fetch(fetchRequest)
                results.forEach {
                    if let bms = $0.bookmarks, bms.count > 0 {
                        // Clear visit count
                        $0.visits = 0
                    } else {
                        // Delete
                        context.delete($0)
                    }
                }
                for obj in results {
                    // Cascading delete on favicon, it will also get deleted
                    context.delete(obj)
                }
            } catch {
                let fetchError = error as NSError
                print(fetchError)
            }

            DataController.save(context: context)
            DispatchQueue.main.async {
                completionOnMain()
            }
        }
    }
}
