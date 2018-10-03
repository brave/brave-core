// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import CoreData
import Shared
import XCGLogger

private let log = Logger.browserLogger

// TODO: Creatable, Updateable. Those are not needed at the moment.
typealias CRUD = Readable & Deletable

public protocol Deletable where Self: NSManagedObject {
    func delete()
    static func deleteAll(predicate: NSPredicate?, context: NSManagedObjectContext,
                          includesPropertyValues: Bool, save: Bool)
}

public protocol Readable where Self: NSManagedObject {
    static func count(predicate: NSPredicate?) -> Int?
    static func first(where predicate: NSPredicate?, context: NSManagedObjectContext) -> Self?
    static func all(where predicate: NSPredicate?, sortDescriptors: [NSSortDescriptor]?, fetchLimit: Int, 
                    context: NSManagedObjectContext) -> [Self]?
}

// MARK: - Implementations
public extension Deletable where Self: NSManagedObject {
    func delete() {
        let context = DataController.newBackgroundContext()
        
        do {
            let objectOnContext = try context.existingObject(with: self.objectID)
            context.delete(objectOnContext)
        
            DataController.save(context: context)
        } catch {
            log.warning("Could not find object: \(self) on a background context.")
        }
    }
    
    static func deleteAll(predicate: NSPredicate? = nil,
                          context: NSManagedObjectContext = DataController.newBackgroundContext(),
                          includesPropertyValues: Bool = true, save: Bool = true) {
        guard let request = getFetchRequest() as? NSFetchRequest<NSFetchRequestResult> else { return }
        request.predicate = predicate
        request.includesPropertyValues = includesPropertyValues
        
        do {
            // NSBatchDeleteRequest can't be used for in-memory store we use in tests.
            // Have to delete objects one by one.
            if AppConstants.IsRunningTest {
                let results = try context.fetch(request) as? [NSManagedObject]
                results?.forEach {
                    context.delete($0)
                }
            } else {
                let deleteRequest = NSBatchDeleteRequest(fetchRequest: request)
                try context.execute(deleteRequest)
            }
        } catch {
            log.error("Delete all error: \(error)")
        }
        
        if save { DataController.save(context: context) }
    }
}

public extension Readable where Self: NSManagedObject { 
    static func count(predicate: NSPredicate? = nil) -> Int? {
        let context = DataController.viewContext
        let request = getFetchRequest()
        
        request.predicate = predicate
        
        do {
            return try context.count(for: request)
        } catch {
            log.error("Count error: \(error)")
        }
        
        return nil
    }
    
    static func first(where predicate: NSPredicate?, context: NSManagedObjectContext = DataController.viewContext) -> Self? {
        return all(where: predicate, fetchLimit: 1, context: context)?.first
    }
    
    static func all(where predicate: NSPredicate? = nil, sortDescriptors: [NSSortDescriptor]? = nil, 
                    fetchLimit: Int = 0, context: NSManagedObjectContext = DataController.viewContext) -> [Self]? {
        let request = getFetchRequest()
        
        request.predicate = predicate
        request.sortDescriptors = sortDescriptors
        request.fetchLimit = fetchLimit
        
        do {
            return try context.fetch(request)
        } catch {
            log.error("Fetch error: \(error)")
        }
        
        return nil
    }
}

// Getting a fetch request for each NSManagedObject
protocol Fetchable: NSFetchRequestResult {}
extension Fetchable {
    static func getFetchRequest() -> NSFetchRequest<Self> {
        var selfName = String(describing: self)
        
        // This is a hack until FaviconMO won't be renamed to Favicon.
        if selfName.contains("FaviconMO") {
            selfName = "Favicon"
        }
        
        return NSFetchRequest<Self>(entityName: selfName)
    }
}
extension NSManagedObject: Fetchable {}
