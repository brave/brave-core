// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import Shared
import BraveShared
import Data
import CoreData

private let log = Logger.browserLogger

class BraveCoreMigrator {
    
    // MARK: Migration State
    
    enum MigrationState {
        case notStarted
        case inProgress
        case failed
        case completed
    }
    
    // MARK: MigrationError
    
    enum MigrationError: LocalizedError {
        case failedBookmarksMigration
        case failedHistoryMigration
        
        public var failureReason: String {
            return Strings.Sync.v2MigrationErrorTitle
        }
        
        public var errorDescription: String {
            return Strings.Sync.v2MigrationErrorMessage
        }
    }
    
    @Observable
    private(set) public var migrationObserver: MigrationState = .notStarted
    
    private var bookmarkMigrationState: MigrationState = .notStarted
    private var historyMigrationState: MigrationState = .notStarted
    
    private var bookmarksAPI: BraveBookmarksAPI?
    private var historyAPI: BraveHistoryAPI?
    
    private let dataImportExporter = BraveCoreImportExportUtility()
    private var bookmarkObserver: BookmarkModelListener?
    private var historyObserver: HistoryServiceListener?
    
    public init() {
        // Check If Chromium Sync Objects Migration is complete (Bookmarks-History)
        if Migration.isChromiumMigrationCompleted {
            migrationObserver = .completed
        }
        
        if let appDelegate = (UIApplication.shared.delegate as? AppDelegate) {
            bookmarksAPI = appDelegate.braveCore?.bookmarksAPI
            historyAPI = appDelegate.braveCore?.historyAPI
        }
        
        #if TEST_MIGRATION
        var didFinishTest = false
        // Add fake bookmarks to CoreData
        self.testMassiveMigration { [weak self] in
            guard let self = self else {
                didFinishTest = true
                return
            }
            
            // Wait for BookmarkModel to Load if needed..
            //
            // If the user is in a sync group, leave the sync chain just in case,
            // so they don't lose everything while testing.
            //
            // Delete all existing BraveCore bookmarks.
            //
            // Finally perform the migration..
            if self.bookmarksAPI.isLoaded {
                BraveSyncAPI.shared.leaveSyncGroup()
                self.bookmarksAPI.removeAll()
                // self.migrate() { _ in
                    didFinishTest = true
                // }
            } else {
                self.bookmarkObserver = self.bookmarksAPI.add(BookmarksModelLoadedObserver({ [weak self] in
                    guard let self = self else { return }
                    self.bookmarkObserver?.destroy()
                    self.bookmarkObserver = nil

                    BraveSyncAPI.shared.leaveSyncGroup()
                    self.bookmarksAPI.removeAll()
                    // self.migrate() { _ in
                        didFinishTest = true
                    // }
                }))
            }
        }
        
        while !didFinishTest {
            RunLoop.current.run(mode: .default, before: .distantFuture)
        }
        
        print("DONE TESTING MIGRATION")
        #endif
    }
    
    public func observeState(from object: AnyObject, _ handler: @escaping (MigrationState, MigrationState) -> Void) {
        _migrationObserver.observe(from: object, handler)
    }
    
    public func migrate(_ completion: ((MigrationError?) -> Void)? = nil) {
        // Check If Chromium Sync Objects Migration is complete (Bookmarks-History)
        if Migration.isChromiumMigrationCompleted {
            migrationObserver = .completed
            completion?(nil)
            return
        }
        
        // Step 1:  Check If bookmarks are migrated / migrate
        migrateBookmarkModels { [unowned self] success in
            guard success else {
                self.migrationObserver = .failed
                completion?(.failedBookmarksMigration)
                
                return
            }
    
            // Step 2: Check If history is migrated / migrate
            migrateHistoryModels { [unowned self] success in
                guard success else {
                    self.migrationObserver = .failed
                    completion?(.failedHistoryMigration)
                    
                    return
                }
                
                completion?(nil)
            }
        }
    }
}

// MARK: Bookmarks Migration

extension BraveCoreMigrator {
    
    private func migrateBookmarkModels(_ completion: @escaping (Bool) -> Void) {
        if !Preferences.Chromium.syncV2BookmarksMigrationCompleted.value {
            // If the bookmark model has already loaded, the observer does NOT get called!
            // Therefore we should continue to migrate the bookmarks
            if bookmarksAPI?.isLoaded == true {
                performBookmarkMigrationIfNeeded { success in
                    completion(success)
                }
            } else {
                // Wait for the bookmark model to load before we attempt to perform migration!
                self.bookmarkObserver = bookmarksAPI?.add(BookmarksModelLoadedObserver({ [weak self] in
                    guard let self = self else { return }
                    self.bookmarkObserver?.destroy()
                    self.bookmarkObserver = nil

                    self.performBookmarkMigrationIfNeeded { success in
                        completion(success)
                    }
                }))
            }
        } else {
            completion(true)
        }
    }
    
    private func performBookmarkMigrationIfNeeded(_ completion: ((Bool) -> Void)?) {
        log.info("Migrating to Chromium Bookmarks v1 - Exporting")
        exportBookmarks { [weak self] success in
            if success {
                log.info("Migrating to Chromium Bookmarks v1 - Start")
                self?.migrateBookmarks() { success in
                    Preferences.Chromium.syncV2BookmarksMigrationCompleted.value = success
                    
                    if let url = BraveCoreMigrator.bookmarksURL {
                        do {
                            try FileManager.default.removeItem(at: url)
                        } catch {
                            log.error("Failed to delete Bookmarks.html backup during Migration")
                        }
                    }
                    
                    completion?(success)
                }
            } else {
                log.info("Migrating to Chromium Bookmarks v1 failed: Exporting")
                completion?(success)
            }
        }
    }
    
    private func migrateBookmarks(_ completion: @escaping (_ success: Bool) -> Void) {
        // Migrate to the mobile folder by default..
        guard let rootFolder = bookmarksAPI?.mobileNode else {
            log.error("Invalid Root Folder - Mobile Node")
            DispatchQueue.main.async {
                completion(false)
            }
            return
        }
        
        DataController.performOnMainContext { context in
            var didSucceed = true
            for bookmark in LegacyBookmarksHelper.getTopLevelLegacyBookmarks(context).sorted(by: { $0.order < $1.order }) {
                if self.migrateChromiumBookmarks(context: context, bookmark: bookmark, chromiumBookmark: rootFolder) {
                    bookmark.delete(context: .existing(context))
                } else {
                    didSucceed = false
                }
            }
            
            DispatchQueue.main.async {
                completion(didSucceed)
            }
        }
    }
    
    private func migrateChromiumBookmarks(context: NSManagedObjectContext, bookmark: LegacyBookmark, chromiumBookmark: BookmarkNode) -> Bool {
        guard let title = bookmark.isFolder ? bookmark.customTitle : bookmark.title else {
            log.error("Invalid Bookmark Title")
            return false
        }
        
        if bookmark.isFolder {
            // Create a folder..
            guard let folder = chromiumBookmark.addChildFolder(withTitle: title) else {
                log.error("Error Creating Bookmark Folder")
                return false
            }
            
            var canDeleteFolder = true
            // Recursively migrate all bookmarks and sub-folders in that root folder..
            // Keeping the original order
            for childBookmark in bookmark.children?.sorted(by: { $0.order < $1.order }) ?? [] {
                if migrateChromiumBookmarks(context: context, bookmark: childBookmark, chromiumBookmark: folder) {
                    childBookmark.delete(context: .existing(context))
                } else {
                    canDeleteFolder = false
                }
            }
            
            if canDeleteFolder {
                bookmark.delete(context: .existing(context))
            }
        } else if let absoluteUrl = bookmark.url, let url = URL(string: absoluteUrl) {
            // Migrate URLs..
            if chromiumBookmark.addChildBookmark(withTitle: title, url: url) == nil {
                log.error("Failed to Migrate Bookmark URL")
                return false
            }
            
            bookmark.delete(context: .existing(context))
        } else {
            return false
        }
        return true
    }
}

// MARK: - History Migration

extension BraveCoreMigrator {
    
    private func migrateHistoryModels(_ completion: @escaping (Bool) -> Void) {
        if !Preferences.Chromium.syncV2HistoryMigrationCompleted.value {
            // If the history model has already loaded, the observer does NOT get called!
            // Therefore we should continue to migrate the history
            if historyAPI?.isBackendLoaded == true {
                performHistoryMigrationIfNeeded { success in
                    completion(success)
                }
            } else {
                // Wait for the history service to load before we attempt to perform migration!
                self.historyObserver = historyAPI?.add(HistoryServiceLoadedObserver({ [weak self] in
                    guard let self = self else { return }
                    self.historyObserver?.destroy()
                    self.historyObserver = nil

                    self.performHistoryMigrationIfNeeded { success in
                        completion(success)
                    }
                }))            }
        } else {
            completion(true)
        }
    }
    
    private func performHistoryMigrationIfNeeded(_ completion: ((Bool) -> Void)?) {
        log.info("Migrating to Chromium History v1 - Start")
        migrateHistory() { success in
            Preferences.Chromium.syncV2HistoryMigrationCompleted.value = success
            completion?(success)
        }
    }
    
    private func migrateHistory(_ completion: @escaping (_ success: Bool) -> Void) {
        DataController.performOnMainContext { context in
            var didSucceed = true
            
            for history in History.fetchMigrationHistory(context) {
                if self.migrateChromiumHistory(context: context, history: history) {
                    history.delete()
                } else {
                    didSucceed = false
                }
            }
            
            DispatchQueue.main.async {
                completion(didSucceed)
            }
        }
    }
    
    private func migrateChromiumHistory(context: NSManagedObjectContext, history: History) -> Bool {
        guard let title = history.title,
              let absoluteUrl = history.url, let url = URL(string: absoluteUrl),
              let dateAdded = history.visitedOn else {
            log.error("Invalid History Specifics")
            return false
        }

        let historyNode = HistoryNode(url: url, title: title, dateAdded: dateAdded)
        historyAPI?.addHistory(historyNode, isURLTyped: true)
        
        return true
    }
}

// MARK: - Bookmarks Export

extension BraveCoreMigrator {
    
    public static var bookmarksURL: URL? {
        let paths = NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)
        guard let documentsDirectory = paths.first else {
            log.error("Unable to access documents directory")
            return nil
        }
        
        guard let url = URL(string: "\(documentsDirectory)/Bookmarks.html") else {
            log.error("Unable to access Bookmarks.html")
            return nil
        }
        
        return url
    }
    
    public static var datedBookmarksURL: URL? {
        let paths = NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)
        guard let documentsDirectory = paths.first else {
            log.error("Unable to access documents directory")
            return nil
        }
        
        let dateFormatter = DateFormatter().then {
            $0.dateFormat = "yyyy-MM-dd_HH:mm:ss"
        }
        
        let dateString = dateFormatter.string(from: Date()).escape() ?? "\(Date().timeIntervalSince1970)"
        
        guard let url = URL(string: "\(documentsDirectory)/Bookmarks_\(dateString).html") else {
            log.error("Unable to access Bookmarks_\(dateString).html")
            return nil
        }
        
        return url
    }
    
    public func exportBookmarks(to url: URL, _ completion: @escaping (_ success: Bool) -> Void) {
        self.dataImportExporter.exportBookmarks(to: url, bookmarks: LegacyBookmarksHelper.getTopLevelLegacyBookmarks().sorted(by: { $0.order < $1.order })) { success in
            completion(success)
        }
    }
    
    private func exportBookmarks(_ completion: @escaping (_ success: Bool) -> Void) {
        guard let url = BraveCoreMigrator.bookmarksURL else {
            return completion(false)
        }
        
        self.dataImportExporter.exportBookmarks(to: url, bookmarks: LegacyBookmarksHelper.getTopLevelLegacyBookmarks().sorted(by: { $0.order < $1.order })) { success in
            completion(success)
        }
    }
}

// MARK: Model Observer

extension BraveCoreMigrator {
    
    class BookmarksModelLoadedObserver: NSObject & BookmarkModelObserver {
        private let onModelLoaded: () -> Void
        
        init(_ onModelLoaded: @escaping () -> Void) {
            self.onModelLoaded = onModelLoaded
        }
        
        func bookmarkModelLoaded() {
            self.onModelLoaded()
        }
    }
    
    class HistoryServiceLoadedObserver: NSObject & HistoryServiceObserver {
        private let onServiceLoaded: () -> Void
        
        init(_ onModelLoaded: @escaping () -> Void) {
            self.onServiceLoaded = onModelLoaded
        }
        
        func historyServiceLoaded() {
            self.onServiceLoaded()
        }
    }
}
