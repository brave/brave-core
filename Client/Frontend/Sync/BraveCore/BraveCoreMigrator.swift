// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveRewards
import Shared
import BraveShared
import Data
import CoreData

private let log = Logger.browserLogger

class BraveCoreMigrator {
    
    @Observable
    private(set) public var migrationObserver: MigrationState = .notStarted
    
    private let dataImportExporter = BraveCoreImportExportUtility()
    private let bookmarksAPI = BraveBookmarksAPI()
    private var observer: BookmarkModelListener?
    
    enum MigrationState {
        case notStarted
        case inProgress
        case failed
        case completed
    }
    
    public init() {
        if Preferences.Chromium.syncV2BookmarksMigrationCompleted.value {
            migrationObserver = .completed
        }
        
        #if TEST_MIGRATION
        var didFinishTest = false
        //Add fake bookmarks to CoreData
        self.testMassiveMigration { [weak self] in
            guard let self = self else {
                didFinishTest = true
                return
            }
            
            //Wait for BookmarkModel to Load if needed..
            //
            //If the user is in a sync group, leave the sync chain just in case,
            //so they don't lose everything while testing.
            //
            //Delete all existing BraveCore bookmarks.
            //
            //Finally perform the migration..
            if self.bookmarksAPI.isLoaded {
                BraveSyncAPI.shared.leaveSyncGroup()
                self.bookmarksAPI.removeAll()
                //self.migrate() { _ in
                    didFinishTest = true
                //}
            } else {
                self.observer = self.bookmarksAPI.add(BookmarksModelLoadedObserver({ [weak self] in
                    guard let self = self else { return }
                    self.observer?.destroy()
                    self.observer = nil

                    BraveSyncAPI.shared.leaveSyncGroup()
                    self.bookmarksAPI.removeAll()
                    //self.migrate() { _ in
                        didFinishTest = true
                    //}
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
    
    public func migrate(_ completion: ((_ success: Bool) -> Void)? = nil) {
        if Preferences.Chromium.syncV2BookmarksMigrationCompleted.value {
            migrationObserver = .completed
            completion?(true)
            return
        }
        
        func performMigrationIfNeeded(_ completion: ((Bool) -> Void)?) {
            if !Preferences.Chromium.syncV2BookmarksMigrationCompleted.value {
                log.info("Migrating to Chromium Bookmarks v1 - Exporting")
                self.exportBookmarks { [weak self] success in
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
            } else {
                completion?(true)
            }
        }
        
        //If the bookmark model has already loaded, the observer does NOT get called!
        //Therefore we should continue to migrate the bookmarks
        if bookmarksAPI.isLoaded {
            performMigrationIfNeeded({
                self.migrationObserver = $0 ? .completed : .failed
                completion?($0)
            })
        } else {
            //Wait for the bookmark model to load before we attempt to perform migration!
            self.observer = bookmarksAPI.add(BookmarksModelLoadedObserver({ [weak self] in
                guard let self = self else { return }
                self.observer?.destroy()
                self.observer = nil

                performMigrationIfNeeded({
                    self.migrationObserver = $0 ? .completed : .failed
                    completion?($0)
                })
            }))
        }
    }
    
    public func exportBookmarks(to url: URL, _ completion: @escaping (_ success: Bool) -> Void) {
        self.dataImportExporter.exportBookmarks(to: url, bookmarks: Bookmark.getAllTopLevelBookmarks().sorted(by: { $0.order < $1.order })) { success in
            completion(success)
        }
    }
    
    private func exportBookmarks(_ completion: @escaping (_ success: Bool) -> Void) {
        guard let url = BraveCoreMigrator.bookmarksURL else {
            return completion(false)
        }
        
        self.dataImportExporter.exportBookmarks(to: url, bookmarks: Bookmark.getAllTopLevelBookmarks().sorted(by: { $0.order < $1.order })) { success in
            completion(success)
        }
    }
    
    private func migrateBookmarks(_ completion: @escaping (_ success: Bool) -> Void) {
        //Migrate to the mobile folder by default..
        guard let rootFolder = bookmarksAPI.mobileNode else {
            log.error("Invalid Root Folder - Mobile Node")
            DispatchQueue.main.async {
                completion(false)
            }
            return
        }
        
        DataController.performOnMainContext { context in
            var didSucceed = true
            for bookmark in Bookmark.getAllTopLevelBookmarks(context).sorted(by: { $0.order < $1.order }) {
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
    
    private func migrateChromiumBookmarks(context: NSManagedObjectContext, bookmark: Bookmark, chromiumBookmark: BookmarkNode) -> Bool {
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
}

extension BraveCoreMigrator {
    private func testMigration(_ completion: @escaping () -> Void) {
        //CODE FOR TESTING MIGRATION!
        //DELETES ALL EXISTING CORE-DATA BOOKMARKS, CREATES A BUNCH OF FAKE BOOKMARKS..
        Preferences.Chromium.syncV2BookmarksMigrationCompleted.value = false

        DataController.perform { context in
            //Delete all existing bookmarks
            Bookmark.getAllTopLevelBookmarks(context).forEach({
                $0.delete(context: .existing(context))
            })

            //TOP LEVEL
            Bookmark.add(url: URL(string: "https://amazon.ca/")!, title: "Amazon", context: .existing(context))
            Bookmark.add(url: URL(string: "https://google.ca/")!, title: "Google", context: .existing(context))

            //TEST FOLDER
            Bookmark.addFolder(title: "TEST", context: .existing(context))

            //TEST -> Brave
            let test = Bookmark.getTopLevelFolders(context).first(where: { $0.customTitle == "TEST" })
            Bookmark.add(url: URL(string: "https://brave.com/")!, title: "Brave", parentFolder: test, context: .existing(context))

            //TEST -> DEPTH Folder
            Bookmark.addFolder(title: "DEPTH", parentFolder: test, context: .existing(context))

            //TEST -> DEPTH -> REDDIT
            let depth = Bookmark.getAllBookmarks(context: context).first(where: { $0.isFolder && $0.parentFolder?.customTitle == "TEST" && $0.customTitle == "DEPTH" })
            Bookmark.add(url: URL(string: "https://reddit.com/")!, title: "Reddit", parentFolder: depth, context: .existing(context))
            
            DispatchQueue.main.async {
                completion()
            }
        }
    }
    
    private func testMassiveMigration(_ completion: @escaping () -> Void) {
        //CODE FOR TESTING MIGRATION!
        //DELETES ALL EXISTING CORE-DATA BOOKMARKS, CREATES A BUNCH OF FAKE BOOKMARKS..
        Preferences.Chromium.syncV2BookmarksMigrationCompleted.value = false

        DataController.perform { context in
            //Delete all existing bookmarks
            Bookmark.getAllTopLevelBookmarks(context).forEach({
                $0.delete(context: .existing(context))
            })
            
            //TEST FOLDER
            Bookmark.addFolder(title: "TEST", context: .existing(context))

            //TEST -> Brave
            let test = Bookmark.getTopLevelFolders(context).first(where: { $0.customTitle == "TEST" })
            
            //TEST -> DEPTH Folder
            Bookmark.addFolder(title: "DEPTH", parentFolder: test, context: .existing(context))
            
            let depth = Bookmark.getAllBookmarks(context: context).first(where: { $0.isFolder && $0.parentFolder?.customTitle == "TEST" && $0.customTitle == "DEPTH" })

            for _ in 0..<400 {
                //TOP LEVEL
                Bookmark.add(url: URL(string: "https://amazon.ca/")!, title: "Amazon", context: .existing(context))
                Bookmark.add(url: URL(string: "https://google.ca/")!, title: "Google", context: .existing(context))

                //TEST ->
                Bookmark.add(url: URL(string: "https://brave.com/")!, title: "Brave", parentFolder: test, context: .existing(context))

                //TEST -> DEPTH ->
                Bookmark.add(url: URL(string: "https://reddit.com/")!, title: "Reddit", parentFolder: depth, context: .existing(context))
            }
            
            DispatchQueue.main.async {
                completion()
            }
        }
    }
}
