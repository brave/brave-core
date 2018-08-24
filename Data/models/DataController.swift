/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import CoreData
import Shared
import XCGLogger

private let log = Logger.browserLogger

public class DataController: NSObject {
    public static var shared: DataController = DataController()
    
    private lazy var container: NSPersistentContainer = {
        let modelName = "Model"
        guard let modelURL = Bundle(for: DataController.self).url(forResource: modelName, withExtension:"momd") else {
            fatalError("Error loading model from bundle")
        }
        guard let mom = NSManagedObjectModel(contentsOf: modelURL) else {
            fatalError("Error initializing managed object model from: \(modelURL)")
        }
        
        let container = NSPersistentContainer(name: modelName, managedObjectModel: mom)
        
        if AppConstants.IsRunningTest {
            let description = NSPersistentStoreDescription()
            description.type = NSInMemoryStoreType
            
            container.persistentStoreDescriptions = [description]
        }
        
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
    
    private var viewContext: NSManagedObjectContext {
        return container.viewContext
    }
    
    public static func save(context: NSManagedObjectContext?) {
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
    
    public static var viewContext: NSManagedObjectContext {
        return DataController.shared.viewContext
    }
    
    public static func newBackgroundContext() -> NSManagedObjectContext {
        let backgroundContext = DataController.shared.container.newBackgroundContext()
        backgroundContext.mergePolicy = NSMergePolicy.mergeByPropertyStoreTrump
        return backgroundContext
    }
}

