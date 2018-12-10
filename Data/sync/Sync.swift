/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import WebKit
import Shared
import CoreData
import SwiftKeychainWrapper
import SwiftyJSON
import JavaScriptCore

private let log = Logger.browserLogger

/*
 module.exports.categories = {
 BOOKMARKS: '0',
 HISTORY_SITES: '1',
 PREFERENCES: '2'
 }
 
 module.exports.actions = {
 CREATE: 0,
 UPDATE: 1,
 DELETE: 2
 }
 */

public let NotificationSyncReady = "NotificationSyncReady"

// TODO: Make capitals - pluralize - call 'categories' not 'type'
public enum SyncRecordType: String {
    case bookmark = "BOOKMARKS"
    case history = "HISTORY_SITES"
    case prefs = "PREFERENCES"
    
    // Please note, this is not a general fetch record string, sync Devices are part of the Preferences
    case devices = "DEVICES"
    //
    
    // These are 'static', and do not change, would make actually lazy/static, but not allow for enums
    var fetchedModelType: SyncRecord.Type? {
        let map: [SyncRecordType: SyncRecord.Type] = [.bookmark: SyncBookmark.self, .prefs: SyncDevice.self]
        return map[self]
    }
    
    var coredataModelType: Syncable.Type? {
        let map: [SyncRecordType: Syncable.Type] = [.bookmark: Bookmark.self, .prefs: Device.self]
        return map[self]
    }
    
    var syncFetchMethod: String {
        return self == .devices ? "fetch-sync-devices" : "fetch-sync-records"
    }
}

public enum SyncObjectDataType: String {
    case Bookmark = "bookmark"
    case Prefs = "preference" // Remove
    
    // Device is considered part of preferences, this is to just be used internally for tracking a constant.
    //  At some point if Sync migrates to further abstracting Device to its own record type, this will be super close
    //  to just working out of the box
    case Device = "device"
}

enum SyncActions: Int {
    case create = 0
    case update = 1
    case delete = 2
    
}

public class Sync: JSInjector {
    
    public static let SeedByteLength = 32
    /// Number of records that is considered a fetch limit as opposed to full data set
    static let RecordFetchAmount = 300
    public static let shared = Sync()
    
    /// This must be public so it can be added into the view hierarchy
    public var webView: WKWebView!
    
    // Should not be accessed directly
    fileprivate var syncReadyLock = false
    var isSyncFullyInitialized = (syncReady: Bool,
                                  fetchReady: Bool,
                                  sendRecordsReady: Bool,
                                  fetchDevicesReady: Bool,
                                  resolveRecordsReady: Bool,
                                  deleteUserReady: Bool,
                                  deleteSiteSettingsReady: Bool,
                                  deleteCategoryReady: Bool)(false, false, false, false, false, false, false, false)
    
    public var isInSyncGroup: Bool {
        return syncSeed != nil
    }
    
    fileprivate var fetchTimer: Timer?
    
    var baseSyncOrder: String? {
        get {
            return UserDefaults.standard.string(forKey: prefBaseOrder)
        }
        set(value) {
            UserDefaults.standard.set(value, forKey: prefBaseOrder)
            UserDefaults.standard.synchronize()
        }
    }
    
    // TODO: Move to a better place
    fileprivate let prefNameId = "device-id-js-array"
    fileprivate let prefNameName = "sync-device-name"
    fileprivate let prefNameSeed = "seed-js-array"
    fileprivate let prefFetchTimestamp = "sync-fetch-timestamp"
    fileprivate let prefBaseOrder = "sync-base-order"
    
    fileprivate lazy var isDebug: Bool = { return !AppConstants.BuildChannel.isRelease }()
    
    fileprivate lazy var serverUrl: String = {
        return isDebug ? "https://sync-staging.brave.com" : "https://sync.brave.com"
    }()
    
    fileprivate let apiVersion = 0
    
    fileprivate var webConfig: WKWebViewConfiguration {
        let webCfg = WKWebViewConfiguration()
        let userController = WKUserContentController()
        
        userController.add(self, name: "syncToIOS_on")
        userController.add(self, name: "syncToIOS_send")
        
        // ios-sync must be called before bundle, since it auto-runs
        let scripts = ["fetch", "ios-sync", "bundle"]
        
        scripts.compactMap { ScriptOpener.get(withName: $0) }.forEach {
            userController.addUserScript(WKUserScript(source: $0, injectionTime: .atDocumentEnd, forMainFrameOnly: true))
        }
        
        webCfg.userContentController = userController
        return webCfg
    }
    
    fileprivate lazy var jsContext: JSContext? = {
        let context = JSContext()
        
        context?.exceptionHandler = { _, exc in
            log.error(exc.debugDescription)
        }
        
        let script = ScriptOpener.get(withName: "bookmark_util")
        context?.evaluateScript(script)
        
        return context
    }()
    
    private var syncFetchedHandlers = [() -> Void]()
    
    override init() {
        super.init()
        
        self.isJavascriptReadyCheck = checkIsSyncReady
        self.maximumDelayAttempts = 15
        self.delayLengthInSeconds = Int64(3.0)
        
        webView = WKWebView(frame: CGRect(x: 30, y: 30, width: 300, height: 500), configuration: webConfig)
        // Attempt sync setup
        initializeSync()
    }
    
    public func leaveSyncGroup() {
        // No, `leaving` logic should be here, any related logic should be in `syncSeed` setter
        syncSeed = nil
    }
    
    func addFetchedHandler(_ handler: @escaping () -> Void) {
        syncFetchedHandlers.append(handler)
    }
    
    /// Sets up sync to actually start pulling/pushing data. This method can only be called once
    /// seed (optional): The user seed, in the form of string hex values. Must be even number : ["00", "ee", "4a", "42"]
    /// Notice:: seed will be ignored if the keychain already has one, a user must disconnect from existing sync group prior to joining a new one
    public func initializeSync(seed: [Int]? = nil, deviceName: String? = nil) {
        
        if let joinedSeed = seed, joinedSeed.count == Sync.SeedByteLength {
            // Always attempt seed write, setter prevents bad overwrites
            syncSeed = "\(joinedSeed)"
        }
        
        // Check to not override deviceName with `nil` on sync init, which happens every app launch
        if let deviceName = deviceName {
            Device.add(name: deviceName, isCurrent: true)
        }
        
        // Autoload sync if already connected to a sync group, otherwise just wait for user initiation
        if let _ = syncSeed {
            self.webView.loadHTMLString("<body>TEST</body>", baseURL: nil)
        }
    }
    
    public func initializeNewSyncGroup(deviceName name: String?) {
        if syncSeed != nil {
            // Error, to setup new sync group, must have no seed
            return
        }
        
        Device.add(name: name, isCurrent: true)
        
        self.webView.loadHTMLString("<body>TEST</body>", baseURL: nil)
    }
    
    fileprivate func webView(_ webView: WKWebView, didFinish navigation: WKNavigation!) {
        print(#function)
    }
    
    fileprivate var syncSeed: String? {
        get {
            if !UserDefaults.standard.bool(forKey: prefNameSeed) {
                // This must be true to stay in sync group
                KeychainWrapper.standard.removeObject(forKey: prefNameSeed)
                return nil
            }
            
            return KeychainWrapper.standard.string(forKey: prefNameSeed)
        }
        set(value) {
            // TODO: Move syncSeed validation here, remove elsewhere
            
            if isInSyncGroup && value != nil {
                // Error, cannot replace sync seed with another seed
                //  must set syncSeed to nil prior to replacing it
                return
            }
            
            if let value = value {
                KeychainWrapper.standard.set(value, forKey: prefNameSeed)
                // Here, we are storing a value to signify a group has been joined
                //  this is _only_ used on a re-installation to know that the app was deleted and re-installed
                UserDefaults.standard.set(true, forKey: prefNameSeed)
                return
            }
            
            // Leave group:
            
            // Clean up group specific items
            // TODO: Update all records with originalSyncSeed
            
            if let device = Device.currentDevice() {
                self.sendSyncRecords(action: .delete, records: [device])
            }
            
            Device.deleteAll()
            
            lastFetchedRecordTimestamp = 0
            lastSuccessfulSync = 0
            lastFetchWasTrimmed = false
            syncReadyLock = false
            isSyncFullyInitialized = (false, false, false, false, false, false, false, false)
            
            fetchTimer?.invalidate()
            fetchTimer = nil
            
            KeychainWrapper.standard.removeObject(forKey: prefNameSeed)
        }
    }
    
    public var syncSeedArray: [Int]? {
        let splitBytes = syncSeed?.components(separatedBy: CharacterSet(charactersIn: "[], ")).filter { !$0.isEmpty }
        let seed = splitBytes?.compactMap { Int($0) }
        return seed?.count == Sync.SeedByteLength ? seed : nil
    }
    
    // TODO: Abstract into classes as static members, each object type needs their own sync time stamp!
    // This includes just the last record that was fetched, used to store timestamp until full process has been completed
    //  then set into defaults
    fileprivate(set) var lastFetchedRecordTimestamp: Int? = 0
    // This includes the entire process: fetching, resolving, insertion/update, and save
    fileprivate var lastSuccessfulSync: Int {
        get {
            return UserDefaults.standard.integer(forKey: prefFetchTimestamp)
        }
        set(value) {
            UserDefaults.standard.set(value, forKey: prefFetchTimestamp)
            UserDefaults.standard.synchronize()
        }
    }
    
    // Same abstraction note as above
    //  Used to know if data on get-existing-objects was trimmed, this value is used inside resolved-sync-records
    fileprivate var lastFetchWasTrimmed: Bool = false
    ////////////////////////////////
    
    @discardableResult func checkIsSyncReady() -> Bool {
        
        if syncReadyLock {
            return true
        }
        
        let mirror = Mirror(reflecting: isSyncFullyInitialized)
        let ready = mirror.children.reduce(true) {
            guard let secondArgAsBool = $1.1 as? Bool else {
                assertionFailure("second argument could not be cast to bool")
                return false
            }
            return $0 && secondArgAsBool
        }
        if ready {
            // Attempt to authorize device
            
            syncReadyLock = true
            
            if let device = Device.currentDevice(), !device.isSynced {
                self.sendSyncRecords(action: .create, records: [device])
                
                // Currently just force this, should use network, but too error prone currently
                Device.currentDevice()?.isSynced = true
                DataController.save(context: Device.currentDevice()?.managedObjectContext)
            }
            
            NotificationCenter.default.post(name: Notification.Name(rawValue: NotificationSyncReady), object: nil)
            
            func startFetching() {
                // Perform first fetch manually
                self.fetchWrapper()
                
                // Fetch timer to run on regular basis
                fetchTimer = Timer.scheduledTimer(timeInterval: 30.0, target: self, selector: #selector(Sync.fetchWrapper), userInfo: nil, repeats: true)
            }
            
            // Just throw by itself, does not need to recover or retry due to lack of importance
            self.fetch(type: .devices)
            
            // Use proper variable and store in defaults
            if lastSuccessfulSync == 0 {
                // Sync local bookmarks, then proceed with fetching
                // Pull all local bookmarks and update their order with newly aquired device id and base sync order.
                let context = DataController.newBackgroundContext()
                
                if let updatedBookmarks = bookmarksWithUpdatedOrder(context: context) {
                    DataController.save(context: context)
                    
                    sendSyncRecords(action: .create, records: updatedBookmarks) { _ in
                        startFetching()
                    }
                }
                
            } else {
                startFetching()
            }
        }
        return ready
    }
    
    fileprivate func bookmarksWithUpdatedOrder(context: NSManagedObjectContext) -> [Bookmark]? {
        guard let deviceId = Device.currentDevice()?.deviceId?.first else { return [] }
        let getBaseBookmarksOrderFunction = jsContext?.objectForKeyedSubscript("getBaseBookmarksOrder")
        
        guard let baseOrder =
            getBaseBookmarksOrderFunction?.call(withArguments: [deviceId, "ios"]).toString() else { return nil }
        
        if baseOrder != "undefined" {
            baseSyncOrder = baseOrder
            return Bookmark.updateBookmarksWithNewSyncOrder(context: context)
        }
        
        return nil
    }
    
    // Required since fetch is wrapped in extension and timer hates that.
    // This can be removed and fetch called directly via scheduledTimerBlock
    @objc func fetchWrapper() {
        self.fetch(type: .bookmark)
        self.fetch(type: .devices)
    }
}

// MARK: Native-initiated Message category
extension Sync {
    // TODO: Rename
    func sendSyncRecords<T: Syncable>(action: SyncActions, records: [T], completion: ((Error?) -> Void)? = nil) {
        
        // Consider protecting against (isSynced && .create)
        
        // Strong typing guarantees that all records are same subclass, so can infer type of all objects
        // Protects against empty record set too
        guard let recordType = records.first?.recordType else {
            completion?(nil)
            return
        }
        
        if !isInSyncGroup {
            completion?(nil)
            return
        }
        
        if recordType == .bookmark && baseSyncOrder == nil {
            log.error("Base sync order is nil.")
            completion?(nil)
            return
        }
        
        executeBlockOnReady() {
            
            // TODO: DeviceId should be sitting on each object already, use that
            let syncRecords = records.map { $0.asDictionary(deviceId: Device.currentDevice()?.deviceId, action: action.rawValue) }
            
            guard let json = JSONSerialization.jsObject(withNative: syncRecords, escaped: false) else {
                // Huge error
                return
            }
            
            /* browser -> webview, sends this to the webview with the data that needs to be synced to the sync server.
             @param {string} categoryName, @param {Array.<Object>} records */
            let evaluate = "callbackList['send-sync-records'](null, '\(recordType.rawValue)',\(json))"
            self.webView.evaluateJavaScript(evaluate,
                                            completionHandler: { (result, error) in
                                                if let error = error {
                                                    print(error)
                                                }
                                                
                                                completion?(error)
            })
        }
    }
    
    func gotInitData() {
        let deviceId = Device.currentDevice()?.deviceId?.description ?? "null"
        let syncSeed = isInSyncGroup ? "new Uint8Array(\(self.syncSeed!))" : "null"
        
        let args = "(null, \(syncSeed), \(deviceId), {apiVersion: '\(apiVersion)', serverUrl: '\(serverUrl)', debug:\(isDebug)})"
        webView.evaluateJavaScript("callbackList['got-init-data']\(args)",
            completionHandler: { (result, error) in
                //                                    print(result)
                //                                    if error != nil {
                //                                        print(error)
                //                                    }
        })
    }
    
    /// Makes call to sync to fetch new records, instead of just returning records, sync sends `get-existing-objects` message
    func fetch(type: SyncRecordType, completion: ((Error?) -> Void)? = nil) {
        /*  browser -> webview: sent to fetch sync records after a given start time from the sync server.
         @param Array.<string> categoryNames, @param {number} startAt (in seconds) **/
        
        executeBlockOnReady() {
            
            // Pass in `lastFetch` to get records since that time
            let evaluate = "callbackList['\(type.syncFetchMethod)'](null, ['\(type.rawValue)'], \(self.lastSuccessfulSync), \(Sync.RecordFetchAmount))"
            self.webView.evaluateJavaScript(evaluate,
                                            completionHandler: { (result, error) in
                                                completion?(error)
            })
        }
    }
    
    func resolvedSyncRecords(_ data: SyncResponse?) {
        
        // TODO: Abstract this logic, same used as in getExistingObjects
        guard let recordJSON = data?.rootElements, let apiRecodType = data?.arg1, let recordType = SyncRecordType(rawValue: apiRecodType) else { return }
        
        guard var fetchedRecords = recordType.fetchedModelType?.syncRecords(recordJSON) else { return }
        
        // Currently only prefs are device related
        if recordType == .prefs, let data = fetchedRecords as? [SyncDevice] {
            // Devices have really bad data filtering, so need to manually process more of it
            // Sort to not rely on API - Reverse sort, so unique pulls the `latest` not just the `first`
            fetchedRecords = data.sorted { device1, device2 in
                device1.syncTimestamp ?? -1 > device2.syncTimestamp ?? -1 }.unique { $0.objectId ?? [] == $1.objectId ?? [] }
            
        }
        
        let context = DataController.newBackgroundContext()
        for fetchedRoot in fetchedRecords {
            
            guard
                let fetchedId = fetchedRoot.objectId
                else { return }
            
            let clientRecord = recordType.coredataModelType?.get(syncUUIDs: [fetchedId], context: context)?.first as? Syncable
            
            var action = SyncActions(rawValue: fetchedRoot.action ?? -1)
            if action == SyncActions.delete {
                clientRecord?.remove(save: false)
            } else if action == SyncActions.create {
                
                if clientRecord != nil {
                    // This can happen pretty often, especially for records that don't use diffs (e.g. prefs>devices)
                    // They always return a create command, even if they already "exist", since there is no real 'resolving'
                    //  Hence check below to prevent duplication
                }
                
                // TODO: Needs favicon
                if clientRecord == nil {
                    _ = recordType.coredataModelType?.add(rootObject: fetchedRoot, save: false, sendToSync: false, context: context)
                } else {
                    // TODO: use Switch with `fallthrough`
                    action = .update
                }
            }
            
            // Handled outside of else block since .create, can modify to an .update
            if action == .update {
                clientRecord?.update(syncRecord: fetchedRoot)
            }
        }
        
        DataController.save(context: context)
        print("\(fetchedRecords.count) \(recordType.rawValue) processed")
        
        // Make generic when other record types are supported
        if recordType != .bookmark {
            // Currently only support bookmark timestamp, so do not want to adjust that
            return
        }
        
        // After records have been written, without issue, save timestamp
        // We increment by a single millisecond to make sure we don't re-fetch the same duplicate records over and over
        // If there are more records with the same timestamp than the batch size, they will be dropped,
        //  however this is unimportant, as it actually prevents an infinitely recursive loop, of refetching the same records over
        //  and over again
        if let stamp = self.lastFetchedRecordTimestamp { self.lastSuccessfulSync = stamp + 1 }
        
        if self.lastFetchWasTrimmed {
            // Do fast refresh, do not wait for timer
            self.fetch(type: .bookmark)
            self.lastFetchWasTrimmed = false
        }
        
        syncFetchedHandlers.forEach { $0() }
    }
    
    func deleteSyncUser(_ data: [String: AnyObject]) {
        print("not implemented: deleteSyncUser() \(data)")
    }
    
    func deleteSyncCategory(_ data: [String: AnyObject]) {
        print("not implemented: deleteSyncCategory() \(data)")
    }
    
    func deleteSyncSiteSettings(_ data: [String: AnyObject]) {
        print("not implemented: delete sync site settings \(data)")
    }
    
}

// MARK: Server To Native Message category
extension Sync {
    
    func getExistingObjects(_ data: SyncResponse?) {
        
        guard let recordJSON = data?.rootElements, let apiRecodType = data?.arg1, let recordType = SyncRecordType(rawValue: apiRecodType) else { return }
        
        guard let fetchedRecords = recordType.fetchedModelType?.syncRecords(recordJSON) else { return }
        
        let ids = fetchedRecords.map { $0.objectId }.compactMap { $0 }
        let localbookmarks = recordType.coredataModelType?.get(syncUUIDs: ids, context: DataController.newBackgroundContext()) as? [Bookmark]
        
        var matchedBookmarks = [[Any]]()
        for fetchedBM in fetchedRecords {
            
            // TODO: Replace with find(where:) in Swift3
            var localBM: Any = "null"
            for l in localbookmarks ?? [] {
                if let localId = l.syncUUID, let fetchedId = fetchedBM.objectId, localId == fetchedId {
                    localBM = l.asDictionary(deviceId: Device.currentDevice()?.deviceId, action: fetchedBM.action)
                    break
                }
            }
            
            matchedBookmarks.append([fetchedBM.dictionaryRepresentation(), localBM])
        }
        
        /* Top level keys: "bookmark", "action","objectId", "objectData:bookmark","deviceId" */
        
        // TODO: Check if parsing not required
        guard let serializedData = JSONSerialization.jsObject(withNative: matchedBookmarks as AnyObject, escaped: false) else {
            // Huge error
            return
        }
        
        // Only currently support bookmarks, this data will be abstracted (see variable definition note)
        if recordType == .bookmark {
            // Store the last record's timestamp, to know what timestamp to pass in next time if this one does not fail
            self.lastFetchedRecordTimestamp = data?.lastFetchedTimestamp
            log.info("sync fetched last timestamp \(self.lastFetchedRecordTimestamp ?? 0)")
            self.lastFetchWasTrimmed = data?.isTruncated ?? false
        }
        
        self.webView.evaluateJavaScript("callbackList['resolve-sync-records'](null, '\(recordType.rawValue)', \(serializedData))",
            completionHandler: { (result, error) in })
    }
    
    // Only called when the server has info for client to save
    func saveInitData(_ data: JSON) {
        // Sync Seed
        if let seedJSON = data["arg1"].array {
            let seed = seedJSON.compactMap({ $0.int })
            
            // TODO: Move to constant
            if seed.count < Sync.SeedByteLength {
                // Error
                return
            }
            
            syncSeed = "\(seed)"
            
        } else if syncSeed == nil {
            // Failure
            print("Seed expected.")
        }
        
        // Device Id
        if let deviceArray = data["arg2"].array, deviceArray.count > 0 {
            // TODO: Just don't set, if bad, allow sync to recover on next init
            Device.currentDevice()?.deviceId = deviceArray.map { $0.intValue }
            DataController.save(context: Device.currentDevice()?.managedObjectContext)
        } else if Device.currentDevice()?.deviceId == nil {
            print("Device Id expected!")
        }
        
    }
    
    func getBookmarkOrder(previousOrder: String?, nextOrder: String?) -> String? {
        
        // Empty string as a parameter means next/previous bookmark doesn't exist
        let prev = previousOrder ?? ""
        let next = nextOrder ?? ""
        
        let getBookmarkOrderFunction = jsContext?.objectForKeyedSubscript("getBookmarkOrder")
        return getBookmarkOrderFunction?.call(withArguments: [prev, next]).toString()
    }
}

extension Sync: WKScriptMessageHandler {
    public func userContentController(_ userContentController: WKUserContentController, didReceive message: WKScriptMessage) {
        
        // JS execution must be on main thread
        
        print("😎 \(message.name) \(message.body)")
        
        let syncResponse = SyncResponse(object: message.body as? String ?? "")
        guard let messageName = syncResponse.message else {
            assert(false)
            return
        }
        
        switch messageName {
        case "get-init-data":
            //            getInitData()
            break
        case "got-init-data":
            self.gotInitData()
        case "save-init-data" :
            // A bit hacky, but this method's data is not very uniform
            // (e.g. arg2 is [Int])
            let data = JSON(parseJSON: message.body as? String ?? "")
            self.saveInitData(data)
            // We clear current sync order after joining a new sync group.
            // syncOrder algorithm is also used for local ordering, even if we are not connected to sync group.
            // After joining a new sync group, new sync order with proper device id must be used.
            Bookmark.removeSyncOrders()
        case "get-existing-objects":
            self.getExistingObjects(syncResponse)
        case "resolved-sync-records":
            self.resolvedSyncRecords(syncResponse)
        case "sync-debug":
            let data = JSON(parseJSON: message.body as? String ?? "")
            print("---- Sync Debug: \(data)")
        case "sync-ready":
            self.isSyncFullyInitialized.syncReady = true
        case "fetch-sync-records":
            self.isSyncFullyInitialized.fetchReady = true
        case "send-sync-records":
            self.isSyncFullyInitialized.sendRecordsReady = true
        case "fetch-sync-devices":
            self.isSyncFullyInitialized.fetchDevicesReady = true
        case "resolve-sync-records":
            self.isSyncFullyInitialized.resolveRecordsReady = true
        case "delete-sync-user":
            self.isSyncFullyInitialized.deleteUserReady = true
        case "delete-sync-site-settings":
            self.isSyncFullyInitialized.deleteSiteSettingsReady = true
        case "delete-sync-category":
            self.isSyncFullyInitialized.deleteCategoryReady = true
        default:
            print("\(messageName) not handled yet")
        }
        
        self.checkIsSyncReady()
    }
}

