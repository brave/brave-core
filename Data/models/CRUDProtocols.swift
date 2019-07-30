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

protocol Deletable where Self: NSManagedObject {
    func delete(context: WriteContext)
    static func deleteAll(predicate: NSPredicate?, context: WriteContext, includesPropertyValues: Bool)
}

protocol Readable where Self: NSManagedObject {
    static func count(predicate: NSPredicate?, context: NSManagedObjectContext) -> Int?
    static func first(where predicate: NSPredicate?, sortDescriptors: [NSSortDescriptor]?, context: NSManagedObjectContext) -> Self?
    static func all(where predicate: NSPredicate?, sortDescriptors: [NSSortDescriptor]?, fetchLimit: Int, 
                    context: NSManagedObjectContext) -> [Self]?
}

// MARK: - Implementations
extension Deletable where Self: NSManagedObject {
    func delete(context: WriteContext = .new(inMemory: false)) {
        
        DataController.perform(context: context) { context in
            let objectOnContext = context.object(with: self.objectID)
            context.delete(objectOnContext)
        }
    }
    
    static func deleteAll(predicate: NSPredicate? = nil,
                          context: WriteContext = .new(inMemory: false),
                          includesPropertyValues: Bool = true) {
        
        DataController.perform(context: context) { context in
            guard let request = getFetchRequest() as? NSFetchRequest<NSFetchRequestResult> else { return }
            request.predicate = predicate
            request.includesPropertyValues = includesPropertyValues
            
            do {
                // NSBatchDeleteRequest can't be used for in-memory store we use in tests and PBM.
                // Have to delete objects one by one.
                var isInMemoryContext: Bool = false
                if let currentCoordinator = context.persistentStoreCoordinator,
                    let inMemoryCoordinator = DataController.viewContextInMemory.persistentStoreCoordinator {
                    isInMemoryContext = currentCoordinator == inMemoryCoordinator
                }
                if AppConstants.IsRunningTest || isInMemoryContext {
                    let results = try context.fetch(request) as? [NSManagedObject]
                    results?.forEach {
                        context.delete($0)
                    }
                } else {
                    let deleteRequest = NSBatchDeleteRequest(fetchRequest: request)
                    // Makes NSBatchDeleteResult to return deleted object IDs.
                    deleteRequest.resultType = .resultTypeObjectIDs

                    // Batch delete writes directly to the persistent store.
                    // Therefore contexts and in-memory objects must be updated manually.
                    let result = try context.execute(deleteRequest) as? NSBatchDeleteResult
                    guard let objectIDArray = result?.result as? [NSManagedObjectID] else { return }
                    let changes = [NSDeletedObjectsKey: objectIDArray]

                    // Merging changes to view context is important because fetch results controllers
                    // listen for changes in this context.
                    // Worker context is also updated in case of performing further operations with it.
                    let contextsToUpdate = [DataController.viewContext, context]
                    NSManagedObjectContext.mergeChanges(fromRemoteContextSave: changes, into: contextsToUpdate)
                }
            } catch {
                log.error("Delete all error: \(error)")
            }
        }
    }
}

extension Readable where Self: NSManagedObject { 
    static func count(predicate: NSPredicate? = nil,
                      context: NSManagedObjectContext = DataController.viewContext) -> Int? {
        let request = getFetchRequest()
        
        request.predicate = predicate
        
        do {
            return try context.count(for: request)
        } catch {
            log.error("Count error: \(error)")
        }
        
        return nil
    }
    
    static func first(where predicate: NSPredicate? = nil, sortDescriptors: [NSSortDescriptor]? = nil,
                      context: NSManagedObjectContext = DataController.viewContext) -> Self? {
        return all(where: predicate, sortDescriptors: sortDescriptors, fetchLimit: 1, context: context)?.first
    }
    
    static func all(where predicate: NSPredicate? = nil,
                    sortDescriptors: [NSSortDescriptor]? = nil,
                    fetchLimit: Int = 0,
                    context: NSManagedObjectContext = DataController.viewContext) -> [Self]? {
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
