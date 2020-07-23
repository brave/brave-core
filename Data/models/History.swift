/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import CoreData
import Shared
import BraveShared

private func getDate(_ dayOffset: Int) -> Date {
    let calendar = Calendar(identifier: Calendar.Identifier.gregorian)
    let nowComponents = calendar.dateComponents([Calendar.Component.year, Calendar.Component.month, Calendar.Component.day], from: Date())
    let today = calendar.date(from: nowComponents)!
    return (calendar as NSCalendar).date(byAdding: NSCalendar.Unit.day, value: dayOffset, to: today, options: [])!
}

private var ignoredSchemes = ["about"]

public func isIgnoredURL(_ url: URL) -> Bool {
    guard let scheme = url.scheme else { return false }

    if let _ = ignoredSchemes.firstIndex(of: scheme) {
        return true
    }

    if url.host == "localhost" {
        return true
    }

    return false
}

public func isIgnoredURL(_ url: String) -> Bool {
    if let url = URL(string: url) {
        return isIgnoredURL(url)
    }

    return false
}

public final class History: NSManagedObject, WebsitePresentable, CRUD {

    @NSManaged public var title: String?
    @NSManaged public var url: String?
    @NSManaged public var visitedOn: Date?
    @NSManaged public var syncUUID: UUID?
    @NSManaged public var domain: Domain?
    @NSManaged public var sectionIdentifier: String?
    
    static let today = getDate(0)
    static let yesterday = getDate(-1)
    static let thisWeek = getDate(-7)
    static let thisMonth = getDate(-31)
    
    // MARK: - Public interface
    
    public override func awakeFromFetch() {
        if sectionIdentifier != nil {
            return
        }
        
        if visitedOn?.compare(History.today) == ComparisonResult.orderedDescending {
            sectionIdentifier = Strings.today
        } else if visitedOn?.compare(History.yesterday) == ComparisonResult.orderedDescending {
            sectionIdentifier = Strings.yesterday
        } else if visitedOn?.compare(History.thisWeek) == ComparisonResult.orderedDescending {
            sectionIdentifier = Strings.lastWeek
        } else {
            sectionIdentifier = Strings.lastMonth
        }
    }

    public class func add(_ title: String, url: URL) {
        DataController.perform { context in
            var item = History.getExisting(url, context: context)
            if item == nil {
                item = History(entity: History.entity(context), insertInto: context)
                item?.domain = Domain.getOrCreateInternal(url, context: context,
                                                          saveStrategy: .delayedPersistentStore)
                item?.url = url.absoluteString
            }
            item?.title = title
            item?.domain?.visits += 1
            item?.visitedOn = Date()
        }
    }

    public class func frc() -> NSFetchedResultsController<History> {
        let fetchRequest = NSFetchRequest<History>()
        let context = DataController.viewContext
        
        fetchRequest.entity = History.entity(context)
        fetchRequest.fetchBatchSize = 20
        fetchRequest.fetchLimit = 200
        fetchRequest.sortDescriptors = [NSSortDescriptor(key: "visitedOn", ascending: false)]
        fetchRequest.predicate = NSPredicate(format: "visitedOn >= %@", History.thisMonth as CVarArg)

        return NSFetchedResultsController(fetchRequest: fetchRequest, managedObjectContext: context, sectionNameKeyPath: "sectionIdentifier", cacheName: nil)
    }
    
    public func delete() {
        delete(context: .new(inMemory: false))
    }

    public class func deleteAll(_ completionOnMain: @escaping () -> Void) {
        DataController.perform { context in
            History.deleteAll(context: .existing(context), includesPropertyValues: false)
            
            Domain.deleteNonBookmarkedAndClearSiteVisits(context: context) {
                completionOnMain()
            }
        }
    }
    
    /// Obtain the last N pages that were visited by the user
    public class func suffix(_ maxLength: Int) throws -> [History] {
        let fetchRequest = NSFetchRequest<History>()
        let context = DataController.viewContext
        
        fetchRequest.entity = History.entity(context)
        fetchRequest.fetchBatchSize = max(20, maxLength)
        fetchRequest.fetchLimit = maxLength
        fetchRequest.sortDescriptors = [NSSortDescriptor(key: "visitedOn", ascending: false)]
        
        return try context.fetch(fetchRequest)
    }
}

// MARK: - Internal implementations

extension History {
    // Currently required, because not `syncable`
    static func entity(_ context: NSManagedObjectContext) -> NSEntityDescription {
        return NSEntityDescription.entity(forEntityName: "History", in: context)!
    }
    
    class func getExisting(_ url: URL, context: NSManagedObjectContext) -> History? {
        let urlKeyPath = #keyPath(History.url)
        let predicate = NSPredicate(format: "\(urlKeyPath) == %@", url.absoluteString)
        
        return first(where: predicate, context: context)
    }
}

extension History: Frecencyable {
    static func byFrecency(query: String? = nil,
                           context: NSManagedObjectContext = DataController.viewContext) -> [WebsitePresentable] {
        let urlKeyPath = #keyPath(History.url)
        let visitedOnKeyPath = #keyPath(History.visitedOn)
        
        var predicate = NSPredicate(format: "\(visitedOnKeyPath) > %@", History.thisWeek as CVarArg)
        if let query = query {
            predicate = NSPredicate(format: predicate.predicateFormat + " AND \(urlKeyPath) CONTAINS %@", query)
        }
        
        return all(where: predicate, fetchLimit: 100, context: context) ?? []
    }
}
