/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import CoreData
import Shared

//      Now that sync is disabled, we hvae fallen back to the original design (from floriankugler)
//      Will update template once issues are ironed out

// After testing many different MOC stacks, it became aparent that main thread context
// should contain no worker children since it will eventually propogate up and block the main
// thread on changes or saves

// Attempting to have the main thread MOC as the sole child of a private MOC seemed optimal 
// (and is recommended path via WWDC Apple CD video), but any associated work on mainMOC
// does not re-merge back into it self well from parent (background) context (tons of issues)
// This should be re-attempted when dropping iOS9, using some of the newer CD APIs for 10+
// (e.g. automaticallyMergesChangesFromParent = true, may allow a complete removal of `merge`)
// StoreCoordinator > writeMOC > mainMOC

// That being said, writeMOC (background) has two parallel children
// One being a mainThreadMOC, and the other a workerMOC. Since contexts seem to have significant
// issues merging their own changes from the parent save, they must merge changes directly from their
// parallel. This seems to work quite well and appears heavily reliable during heavy background work.
// StoreCoordinator > writeMOC (private, no direct work) > mainMOC && workerMOC

// Previoulsy attempted stack which had significant impact on main thread saves
// Follow the stack design from http://floriankugler.com/2013/04/02/the-concurrent-core-data-stack/

class DataController: NSObject {
    static let shared = DataController()
    
    fileprivate lazy var writeContext: NSManagedObjectContext = {
        let write = NSManagedObjectContext(concurrencyType: .privateQueueConcurrencyType)
        write.persistentStoreCoordinator = self.persistentStoreCoordinator
        write.undoManager = nil
        write.mergePolicy = NSMergeByPropertyStoreTrumpMergePolicy
        
        return write
    }()
    
    lazy var workerContext: NSManagedObjectContext = {
    
        let worker = NSManagedObjectContext(concurrencyType: .privateQueueConcurrencyType)
        worker.undoManager = nil
        worker.mergePolicy = NSOverwriteMergePolicy
        worker.parent = self.writeContext
        worker.automaticallyMergesChangesFromParent = true
        
        return worker
    }()
    
    lazy var mainThreadContext: NSManagedObjectContext = {
        let main = NSManagedObjectContext(concurrencyType: .mainQueueConcurrencyType)
        main.undoManager = nil
        main.mergePolicy = NSOverwriteMergePolicy
        main.parent = self.writeContext
        main.automaticallyMergesChangesFromParent = true
        
        return main
    }()
    
    fileprivate var managedObjectModel: NSManagedObjectModel!
    fileprivate var persistentStoreCoordinator: NSPersistentStoreCoordinator!
    
    fileprivate override init() {
        super.init()

       // TransformerUUID.setValueTransformer(transformer: NSValueTransformer?, forName name: String)

        guard let modelURL = Bundle.main.url(forResource: "Model", withExtension:"momd") else {
            fatalError("Error loading model from bundle")
        }
        guard let mom = NSManagedObjectModel(contentsOf: modelURL) else {
            fatalError("Error initializing mom from: \(modelURL)")
        }
        
        self.managedObjectModel = mom
        self.persistentStoreCoordinator = NSPersistentStoreCoordinator(managedObjectModel: managedObjectModel)
        
        let urls = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)
        if let docURL = urls.last {
            do {
                
                let options: [String: AnyObject] = [
                    NSMigratePersistentStoresAutomaticallyOption: true as AnyObject,
                    NSInferMappingModelAutomaticallyOption: true as AnyObject,
                    NSPersistentStoreFileProtectionKey : FileProtectionType.complete as AnyObject
                ]
                
                // Old store URL from old beta, can be removed at some point (thorough migration testing though)
                var storeURL = docURL.appendingPathComponent("Brave.sqlite")
                try self.persistentStoreCoordinator.addPersistentStore(ofType: NSSQLiteStoreType, configurationName: nil, at: storeURL, options: options)
                
                storeURL = docURL.appendingPathComponent("Model.sqlite")
                try self.persistentStoreCoordinator.addPersistentStore(ofType: NSSQLiteStoreType, configurationName: nil, at: storeURL, options: options)
            }
            catch {
                fatalError("Error migrating store: \(error)")
            }
        }

        // Setup contexts
        _ = mainThreadContext
    }
    
    static func remove(object: NSManagedObject, context: NSManagedObjectContext = DataController.shared.mainThreadContext) {
        context.delete(object)
        DataController.saveContext(context: context)
    }

    static func saveContext(context: NSManagedObjectContext?) {
        guard let context = context else {
            print("No context on save")
            return
        }
        
        if context === DataController.shared.writeContext {
            print("Do not use with the write moc, this save is handled internally here.")
            return
        }

        // TODO: Clean this up
        context.perform {
            if !context.hasChanges {
                return
            }
            
            do {
                try context.save()
                
                DataController.shared.writeContext.perform {
                    if !DataController.shared.writeContext.hasChanges {
                        return
                    }
                    do {
                        try DataController.shared.writeContext.save()
                    } catch {
                        fatalError("Error saving DB to disk: \(error)")
                    }
                }
            } catch {
                fatalError("Error saving DB: \(error)")
            }
        }
    }
}

extension NSManagedObjectContext {
    static var mainThreadContext: NSManagedObjectContext {
        return DataController.shared.mainThreadContext
    }
    
    static var workerThreadContext: NSManagedObjectContext {
        return DataController.shared.workerContext
    }
}
