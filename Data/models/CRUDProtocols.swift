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
    static func deleteAll(predicate: NSPredicate?)
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
        let context = self.managedObjectContext ?? DataController.backgroundContext
        
        context.perform {
            context.delete(self)
            if !context.hasChanges {
                log.debug("Wanting to remove object \(self) but context registered no changes.")
                return
            }
            
            do {
                try context.save()
            } catch {
                log.debug("Failed to save context after removing object \(self), error: \(error)")
            }
        }
    }
    
    static func deleteAll(predicate: NSPredicate? = nil) {
        let context = DataController.backgroundContext
        let request = getFetchRequest()
        
        request.predicate = predicate
        
        do {
            let results = try context.fetch(request)
            results.forEach {
                context.delete($0)
            }
            
            DataController.save(context: context)
        } catch {
            log.error("Delete all error: \(error)")
        }
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
        return NSFetchRequest<Self>(entityName: String(describing: self))
    }
}
extension NSManagedObject: Fetchable {}
