/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import CoreData
import Shared
import XCGLogger
import BraveShared

private let log = Logger.browserLogger

/// A helper structure for `DataController.perform()` method
/// to decide whether a new or existing context should be used
/// to perform a database write operation.
public enum WriteContext {
    /// Requests DataController to create new background context for the task.
    case new(inMemory: Bool)
    /// Requests DataController to use an existing context.
    /// (To prevent creating multiple contexts per call and mixing threads)
    case existing(_ context: NSManagedObjectContext)
}

public class DataController {
    private static let databaseName = "Brave.sqlite"
    private static let modelName = "Model"
    
    /// This code is checked when the persistent store is loaded.
    /// For all codes except this one we crash the app because of database failure.
    private static let storeExistsErrorCode = 134081
    
    // MARK: - Initialization
    
    /// Managed Object Model of the database stack.
    /// Must be created only once, this is to prevent a bug when testing with in-memory store.
    /// More info here https://stackoverflow.com/a/51857486.
    /// Note: this might be not needed in Swift 5.1 or newer.
    private static let model: NSManagedObjectModel = {
        guard let modelURL = Bundle(for: DataController.self).url(forResource: modelName, withExtension: "momd") else {
            fatalError("Error loading model from bundle")
        }
        guard let mom = NSManagedObjectModel(contentsOf: modelURL) else {
            fatalError("Error initializing managed object model from: \(modelURL)")
        }
        
        return mom
    }()
    
    private let operationQueue: OperationQueue = {
        let queue = OperationQueue()
        queue.maxConcurrentOperationCount = 1
        return queue
    }()
    
    private var initializationCompleted = false
    
    /// IMPORTANT: This must be called after pre 1.12 migration logic has been called.
    /// Initialization logic will run only once, then do nothing on subsequent calls to this method.
    public func initializeOnce() {
        if initializationCompleted { return }
        
        configureContainer(container, store: storeURL)
        createOldDocumentStoreIfNeeded()
        initializationCompleted = true
    }
    
    // MARK: - Public interface
    
    public static var shared: DataController = DataController()
    public static var sharedInMemory: DataController = InMemoryDataController()
    
    public func storeExists() -> Bool {
        return FileManager.default.fileExists(atPath: storeURL.path)
    }
    
    private let container = NSPersistentContainer(name: DataController.modelName,
                                                  managedObjectModel: DataController.model)
    
    // MARK: - Old Database migration methods
    
    /// Returns old pre 1.12 persistent container or nil if it doesn't exist on the device.
    public var oldDocumentStore: NSPersistentContainer?
    private var migrationContainer: NSPersistentContainer?
    
    private func createOldDocumentStoreIfNeeded() {
        let fm = FileManager.default
        guard let urls = fm.urls(for: FileManager.SearchPathDirectory.documentDirectory,
                                 in: .userDomainMask).last else { return }
        
        let name = DataController.databaseName
        let path = urls.appendingPathComponent(name).path
        
        if fm.fileExists(atPath: path) {
            migrationContainer = NSPersistentContainer(name: DataController.modelName, managedObjectModel: DataController.model)
            if let migrationContainer = migrationContainer {
                configureContainer(migrationContainer, store: oldDocumentStoreURL)
            }
        }
    }
    
    public var newStoreExists: Bool {
        let fm = FileManager.default
        guard let urls = fm.urls(for: FileManager.SearchPathDirectory.applicationSupportDirectory,
                                 in: .userDomainMask).last else { return false }
        
        let name = DataController.databaseName
        let path = urls.appendingPathComponent(name).path
        
        return fm.fileExists(atPath: path)
    }
    
    public func migrateToNewPathIfNeeded() throws {
        enum MigrationError: Error {
            case OldStoreMissing(String)
            case MigrationFailed(String)
            case CleanupFailed(String)
        }
        
        // This logic must account for 4 different situations:
        // 1. New Users (no migration, use new location
        // 2. Upgraded users with successful migration (use new database)
        // 3. Upgraded users with unsuccessful migrations (use new database, ignore old files)
        // 4. Upgrading users (attempt migration, if fail, use old store, if successful delete old files)
        //      - re-attempt migration on every new app version, until they are in #2
        
        if oldDocumentStore == nil || newStoreExists {
            // Old store absent, no data to migrate (#1 | #3)
            // or
            // New store already exists, do not attempt to overwrite (#2)
            
            // Update flag to avoid re-running this logic
            Preferences.Database.DocumentToSupportDirectoryMigration.completed.value = true
            return
        }
        
        // Going to attempt migration (#4 in some level)
        guard let migrationContainer = migrationContainer else {
            throw MigrationError.OldStoreMissing("Migration container missing")
        }
        
        let coordinator = migrationContainer.persistentStoreCoordinator

        guard let oldStore = coordinator.persistentStore(for: oldDocumentStoreURL) else {
            throw MigrationError.OldStoreMissing("Old store unavailable")
        }
        
        // Attempting actual database migration Document -> Support 🤞
        do {
            let migrationOptions = [
                NSPersistentStoreFileProtectionKey: true
            ]
            try coordinator.migratePersistentStore(oldStore, to: supportStoreURL, options: migrationOptions, withType: NSSQLiteStoreType)
        } catch {
            throw MigrationError.MigrationFailed("Document -> Support database migration failed: \(error)")
            // Migration failed somehow, and old store is present. Flag not being updated 😭
        }
        
        // Regardless of cleanup logic, the actual migration was successful, so we're just going for it 🙀😎
        Preferences.Database.DocumentToSupportDirectoryMigration.completed.value = true
        
        // Cleanup time 🧹
        do {
            try coordinator.destroyPersistentStore(at: oldDocumentStoreURL, ofType: NSSQLiteStoreType, options: nil)
            
            let documentFiles = try FileManager.default.contentsOfDirectory(
                at: oldDocumentStoreURL.deletingLastPathComponent(),
                includingPropertiesForKeys: nil,
                options: [])
            
            // Delete all Brave.X files
            try documentFiles
                .filter {$0.lastPathComponent.hasPrefix(DataController.databaseName)}
                .forEach(FileManager.default.removeItem)
        } catch {
            throw MigrationError.CleanupFailed("Document -> Support database cleanup failed: \(error)")
            // Do not re-point store, as the migration was successful, just the clean up failed
        }
        
        // At this point, everything was a pure success 👏
    }
    
    /// Warning! Please use `storeURL`. This is for migration purpose only.
    private var oldDocumentStoreURL: URL {
        return storeURL(for: FileManager.SearchPathDirectory.documentDirectory)
    }
    
    /// Warning! Please use `storeURL`. This is for migration purposes only.
    private var supportStoreURL: URL {
        return storeURL(for: FileManager.SearchPathDirectory.applicationSupportDirectory)
    }
    
    private func storeURL(for directory: FileManager.SearchPathDirectory) -> URL {
        let urls = FileManager.default.urls(for: directory, in: .userDomainMask)
        guard let docURL = urls.last else {
            log.error("Could not load url for: \(directory)")
            fatalError()
        }
        
        return docURL.appendingPathComponent(DataController.databaseName)
    }
    
    // MARK: - Data framework interface
    
    static func perform(context: WriteContext = .new(inMemory: false), save: Bool = true,
                        task: @escaping (NSManagedObjectContext) -> Void) {
        
        switch context {
        case .existing(let existingContext):
            // If existing context is provided, we only call the code closure.
            // Queue operation and saving is done in `performTask()`
            // called at higher level when a `.new` WriteContext is passed.
            task(existingContext)
        case .new(let inMemory):
            // Though keeping same queue does not make a difference but kept them diff for independent processing.
            let queue = inMemory ? DataController.sharedInMemory.operationQueue :  DataController.shared.operationQueue
            
            queue.addOperation({
                let backgroundContext = inMemory ? DataController.newBackgroundContextInMemory() : DataController.newBackgroundContext()
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
    
    public static func performOnMainContext(save: Bool = true, task: @escaping (NSManagedObjectContext) -> Void) {
        self.perform(context: .existing(self.viewContext), save: save, task: task)
    }
    
    // Context object also allows us access to all persistent container data if needed.
    static var viewContext: NSManagedObjectContext {
        return DataController.shared.container.viewContext
    }
    
    // Context object also allows us access to all persistent container data if needed.
    static var viewContextInMemory: NSManagedObjectContext {
        return DataController.sharedInMemory.container.viewContext
    }
    
    func addPersistentStore(for container: NSPersistentContainer, store: URL) {
        let storeDescription = NSPersistentStoreDescription(url: store)
        
        // This makes the database file encrypted until first user unlock after device restart.
        let completeProtection = FileProtectionType.completeUntilFirstUserAuthentication as NSObject
        storeDescription.setOption(completeProtection, forKey: NSPersistentStoreFileProtectionKey)
        
        container.persistentStoreDescriptions = [storeDescription]
    }
    
    var storeURL: URL {
        let supportDirectory = Preferences.Database.DocumentToSupportDirectoryMigration.completed.value
        return supportDirectory ? supportStoreURL : oldDocumentStoreURL
    }
    
    private func configureContainer(_ container: NSPersistentContainer, store: URL) {
        addPersistentStore(for: container, store: store)
        
        container.loadPersistentStores(completionHandler: { store, error in
            if let error = error {
                // Do not crash the app if the store already exists.
                if (error as NSError).code != Self.storeExistsErrorCode {
                    fatalError("Load persistent store error: \(error)")
                }
            }
            
            if store.type != NSInMemoryStoreType {
                // This makes the database file encrypted until first user unlock after device restart.
                let completeProtection = FileProtectionType.completeUntilFirstUserAuthentication as NSObject
                store.setOption(completeProtection, forKey: NSPersistentStoreFileProtectionKey)
            }
        })
        // We need this so the `viewContext` gets updated on changes from background tasks.
        container.viewContext.automaticallyMergesChangesFromParent = true
    }
    
    static func newBackgroundContext() -> NSManagedObjectContext {
        let backgroundContext = DataController.shared.container.newBackgroundContext()
        // In theory, the merge policy should not matter
        // since all operations happen on a synchronized operation queue.
        // But in case of any bugs it's better to have one, so the app won't crash for users.
        backgroundContext.mergePolicy = NSMergePolicy.mergeByPropertyStoreTrump
        return backgroundContext
    }
    
    static func newBackgroundContextInMemory() -> NSManagedObjectContext {
        let backgroundContext = DataController.sharedInMemory.container.newBackgroundContext()
        backgroundContext.mergePolicy = NSMergePolicy.mergeByPropertyStoreTrump
        return backgroundContext
    }
}

