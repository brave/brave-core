/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import CoreData
import Shared
import XCGLogger

private let log = Logger.browserLogger

/// A helper structure for `DataController.perform()` method
/// to decide whether a new or existing context should be used
/// to perform a database write operation.
enum WriteContext {
    /// Requests DataController to create new background context for the task.
    case new
    /// Requests DataController to use an existing context.
    /// (To prevent creating multiple contexts per call and mixing threads)
    case existing(_ context: NSManagedObjectContext)
}

public class DataController: NSObject {
    private static let databaseName = "Brave.sqlite"
    
    // MARK: - Public interface
    
    public static var shared: DataController = DataController()
    
    public func storeExists() -> Bool {
        return FileManager.default.fileExists(atPath: storeURL.path)
    }
    
    // MARK: - Data framework interface
    
    static func perform(context: WriteContext = .new, save: Bool = true,
                        task: @escaping (NSManagedObjectContext) -> Void) {
        
        switch context {
        case .existing(let existingContext):
            // If existing context is provided, we only call the code closure.
            // Queue operation and saving is done in `performTask()`
            // called at higher level when a `.new` WriteContext is passed.
            task(existingContext)
        case .new:
            let queue = DataController.shared.operationQueue
            
            queue.addOperation({
                let backgroundContext = DataController.newBackgroundContext()
                // performAndWait doesn't block main thread because it fires on OperationQueue`s background thread.
                backgroundContext.performAndWait {
                    task(backgroundContext)
                    
                    guard save && backgroundContext.hasChanges else { return }
                    
                    do {
                        assert(!Thread.isMainThread)
                        try backgroundContext.save()
                    } catch {
                        log.error("performTask save error: \(error)")
                    }
                }
            })
        }
    }
    
    // Context object also allows us access to all persistent container data if needed.
    static var viewContext: NSManagedObjectContext {
        return DataController.shared.container.viewContext
    }
    
    static func save(context: NSManagedObjectContext?) {
        guard let context = context else {
            log.warning("No context on save")
            return
        }
        
        if context == DataController.viewContext {
            log.warning("Writing to view context, this should be avoided.")
        }
        
        context.perform {
            if !context.hasChanges { return }
            
            do {
                try context.save()
            } catch {
                assertionFailure("Error saving DB: \(error)")
            }
        }
    }
    
    func addPersistentStore(for container: NSPersistentContainer) {
        let storeDescription = NSPersistentStoreDescription(url: storeURL)
        
        // This makes the database file encrypted until device is unlocked.
        let completeProtection = FileProtectionType.complete as NSObject
        storeDescription.setOption(completeProtection, forKey: NSPersistentStoreFileProtectionKey)
        
        container.persistentStoreDescriptions = [storeDescription]
    }
    
    // MARK: - Private
    
    private lazy var container: NSPersistentContainer = {
        
        let modelName = "Model"
        guard let modelURL = Bundle(for: DataController.self).url(forResource: modelName, withExtension: "momd") else {
            fatalError("Error loading model from bundle")
        }
        guard let mom = NSManagedObjectModel(contentsOf: modelURL) else {
            fatalError("Error initializing managed object model from: \(modelURL)")
        }
        
        let container = NSPersistentContainer(name: modelName, managedObjectModel: mom)
        
        addPersistentStore(for: container)
        
        // Dev note: This completion handler might be misleading: the persistent store is loaded synchronously by default.
        container.loadPersistentStores(completionHandler: { _, error in
            if let error = error {
                fatalError("Load persistent store error: \(error)")
            }
        })
        // We need this so the `viewContext` gets updated on changes from background tasks.
        container.viewContext.automaticallyMergesChangesFromParent = true
        return container
    }()
    
    private lazy var operationQueue: OperationQueue = {
        let queue = OperationQueue()
        queue.maxConcurrentOperationCount = 1
        return queue
    }()
    
    private let storeURL: URL = {
        let urls = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)
        guard let docURL = urls.last else {
            log.error("Could not load url at document directory")
            fatalError()
        }
        
        return docURL.appendingPathComponent(DataController.databaseName)
    }()
    
    private static func newBackgroundContext() -> NSManagedObjectContext {
        let backgroundContext = DataController.shared.container.newBackgroundContext()
        // In theory, the merge policy should not matter
        // since all operations happen on a synchronized operation queue.
        // But in case of any bugs it's better to have one, so the app won't crash for users.
        backgroundContext.mergePolicy = NSMergePolicy.mergeByPropertyStoreTrump
        return backgroundContext
    }
}
