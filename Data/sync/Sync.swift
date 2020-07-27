/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import WebKit
import Shared
import BraveShared
import CoreData
import SwiftKeychainWrapper
import SwiftyJSON
import JavaScriptCore

private let log = Logger.braveSyncLogger

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
    
    /// Preferences(devices) have separate fetch timestamp.
    var lastFetchTimeStamp: Int {
        switch self {
        case .prefs: return Preferences.Sync.lastPreferenceFetchTimestamp.value
        default: return Preferences.Sync.lastFetchTimestamp.value
        }
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
    
    public struct Notifications {
        public static let syncReady = Notification.Name(rawValue: "NotificationSyncReady")
        public static let didLeaveSyncGroup = Notification.Name(rawValue: "NotificationLeftSyncGroup")
    }
    
    public static let seedByteLength = 32
    /// Number of records that is considered a fetch limit as opposed to full data set
    static let recordFetchAmount = 300
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
    
    let syncFetchMethod = "fetch-sync-records"
    let fetchInterval: TimeInterval = 60
    
    fileprivate var fetchTimer: Timer?
    
    /// If sync initialization fails, we should inform user and remove all partial sync setup that happened.
    /// Please note that sync initialization also happens on app launch, not only on first connection to Sync.
    public var syncSetupFailureCallback: (() -> Void)?
    
    fileprivate lazy var isDebug: Bool = { return AppConstants.buildChannel == .debug }()
    
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
    
    private var baseSyncOrder: String { return Preferences.Sync.baseSyncOrder.value }
    
    override init() {
        super.init()
        
        self.isJavascriptReadyCheck = checkIsSyncReady
        self.maximumDelayAttempts = 15
        self.delayLengthInSeconds = Int64(3.0)
        
        DispatchQueue.main.async {
            self.webView = WKWebView(frame: CGRect(x: 30, y: 30, width: 300, height: 500), configuration: self.webConfig)
            // Attempt sync setup
            self.initializeSync()
        }
    }
    
    public func leaveSyncGroup(sendToSync: Bool = true) {
        leaveSyncGroupInternal(sendToSync: sendToSync)
    }
    
    func leaveSyncGroupInternal(sendToSync: Bool, context: WriteContext = .new(inMemory: false)) {
        syncSeed = nil
        
        DataController.perform(context: context) { context in
            // Leaving sync group can be triggered either on own device, or remotely, if another device in the sync chain
            // will remove this device. In the second case we don't want to send deleted device sync record because it
            // was already deleted and can cause an infinite loop.
            if let device = Device.currentDevice(context: context), sendToSync {
                self.sendSyncRecords(action: .delete, records: [device], context: context)
            }
            
            Device.sharedCurrentDeviceId = nil
            Device.deleteAll(context: .existing(context))
        }
        
        Preferences.Sync.lastFetchTimestamp.reset()
        Preferences.Sync.lastPreferenceFetchTimestamp.reset()
        lastFetchWasTrimmed = false
        syncReadyLock = false
        isSyncFullyInitialized = (false, false, false, false, false, false, false, false)
        
        fetchTimer?.invalidate()
        fetchTimer = nil
        
        KeychainWrapper.standard.removeObject(forKey: Preferences.Sync.seedName.key)
        // If a user stays on any of Sync screens while another device removes this device
        // we have to dismiss the Sync view controller.
        NotificationCenter.default.post(name: Sync.Notifications.didLeaveSyncGroup, object: nil)
    }
    
    func addFetchedHandler(_ handler: @escaping () -> Void) {
        syncFetchedHandlers.append(handler)
    }
    
    /// Sets up sync to actually start pulling/pushing data. This method can only be called once
    /// seed (optional): The user seed, in the form of string hex values. Must be even number : ["00", "ee", "4a", "42"]
    /// Notice:: seed will be ignored if the keychain already has one, a user must disconnect from existing sync group prior to joining a new one
    public func initializeSync(seed: [Int]? = nil, deviceName: String? = nil) {
        
        if let joinedSeed = seed, joinedSeed.count == Sync.seedByteLength {
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
            let seedName = Preferences.Sync.seedName.key
            if !Preferences.Sync.seedName.value {
                // This must be true to stay in sync group
                KeychainWrapper.standard.removeObject(forKey: seedName)
                return nil
            }
            
            return KeychainWrapper.standard.string(forKey: seedName)
        }
        set(value) {
            let seedName = Preferences.Sync.seedName.key
            
            // TODO: Move syncSeed validation here, remove elsewhere
            
            if isInSyncGroup && value != nil {
                log.error("Error, cannot replace sync seed with another seed must set syncSeed to nil prior to replacing it")
                return
            }
            
            if let value = value {
                KeychainWrapper.standard.set(value, forKey: seedName)
                Preferences.Sync.seedName.value = true
                return
            }
        }
    }
    
    public var syncSeedArray: [Int]? {
        let splitBytes = syncSeed?.components(separatedBy: CharacterSet(charactersIn: "[], ")).filter { !$0.isEmpty }
        let seed = splitBytes?.compactMap { Int($0) }
        return seed?.count == Sync.seedByteLength ? seed : nil
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
            
            NotificationCenter.default.post(name: Sync.Notifications.syncReady, object: nil)
            
            func startFetching() {
                // Assigning the fetch timer must run on main thread.
                DispatchQueue.main.async {
                    // Perform first fetch manually
                    self.fetchWrapper()
                    
                    // Fetch timer to run on regular basis
                    self.fetchTimer = Timer.scheduledTimer(timeInterval: self.fetchInterval, target: self, selector: #selector(Sync.fetchWrapper), userInfo: nil, repeats: true)
                }
            }
            
            // Just throw by itself, does not need to recover or retry due to lack of importance
            self.fetch(type: .prefs)
            
            // Use proper variable and store in defaults
            if Preferences.Sync.lastFetchTimestamp.value == 0 {
                // Sync local bookmarks, then proceed with fetching
                // Pull all local bookmarks and update their order with newly aquired device id and base sync order.
                DataController.perform { context in
                    if let updatedBookmarks = self.bookmarksWithUpdatedOrder(context: context) {
                        
                        self.sendSyncRecords(action: .create, records: updatedBookmarks, context: context) { _ in
                            startFetching()
                        }
                    }
                }
                
            } else {
                startFetching()
            }
        }
        return ready
    }
    
    fileprivate func bookmarksWithUpdatedOrder(context: NSManagedObjectContext) -> [Bookmark]? {
        guard let deviceId = Device.currentDevice(context: context)?.deviceId?.first else { return [] }
        let getBaseBookmarksOrderFunction = jsContext?.objectForKeyedSubscript("getBaseBookmarksOrder")
        
        guard let baseOrder =
            getBaseBookmarksOrderFunction?.call(withArguments: [deviceId, "ios"]).toString() else { return nil }
        
        if baseOrder != "undefined" {
            Preferences.Sync.baseSyncOrder.value = baseOrder
            return Bookmark.updateBookmarksWithNewSyncOrder(context: context)
        }
        
        return nil
    }
    
    // Required since fetch is wrapped in extension and timer hates that.
    // This can be removed and fetch called directly via scheduledTimerBlock
    @objc func fetchWrapper() {
        self.fetch(type: .bookmark)
        self.fetch(type: .prefs)
    }
}

// MARK: Native-initiated Message category
extension Sync {
    // TODO: Rename
    func sendSyncRecords<T: Syncable>(action: SyncActions, records: [T],
                                      context: NSManagedObjectContext = DataController.viewContext,
                                      completion: ((Error?) -> Void)? = nil) {
        
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
        
        // TODO: DeviceId should be sitting on each object already, use that
        let syncRecords = records.map { $0.asDictionary(deviceId:
            Device.currentDevice(context: context)?.deviceId, action: action.rawValue) }
        
        executeBlockOnReady() {
            guard let json = JSONSerialization.jsObject(withNative: syncRecords, escaped: false) else {
                // Huge error
                return
            }
            
            /* browser -> webview, sends this to the webview with the data that needs to be synced to the sync server.
             @param {string} categoryName, @param {Array.<Object>} records */
            let evaluate = "callbackList['send-sync-records'](null, '\(recordType.rawValue)',\(json))"
            DispatchQueue.main.async {
                self.webView.evaluateJavaScript(evaluate,
                                                completionHandler: { (result, error) in
                                                    if let error = error {
                                                        log.error(error)
                                                    }
                                                    
                                                    completion?(error)
                })
            }
            
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
        
        leaveSyncGroup()
    }
    
    /// Makes call to sync to fetch new records, instead of just returning records, sync sends `get-existing-objects` message
    func fetch(type: SyncRecordType, completion: ((Error?) -> Void)? = nil) {
        /*  browser -> webview: sent to fetch sync records after a given start time from the sync server.
         @param Array.<string> categoryNames, @param {number} startAt (in seconds) **/
        
        executeBlockOnReady() {
            
            // Pass in `lastFetch` to get records since that time
            let evaluate = "callbackList['\(self.syncFetchMethod)'](null, ['\(type.rawValue)'], \(type.lastFetchTimeStamp), \(Sync.recordFetchAmount))"
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
        if recordType == .prefs {
            // Devices have really bad data filtering, so need to manually process more of it
            // Sort to not rely on API - Reverse sort, so unique pulls the `latest` not just the `first`
            fetchedRecords = fetchedRecords.sorted { device1, device2 in
                device1.syncTimestamp ?? -1 > device2.syncTimestamp ?? -1 }.unique { $0.objectId ?? [] == $1.objectId ?? [] }
            
        }
        
        DataController.perform { context in
            for fetchedRoot in fetchedRecords {
                
                guard let fetchedId = fetchedRoot.objectId else { return }
                
                let clientRecord = recordType.coredataModelType?.get(syncUUIDs: [fetchedId], context: context)?.first as? Syncable
                
                var action = SyncActions(rawValue: fetchedRoot.action ?? -1)
                if action == SyncActions.delete {
                    clientRecord?.deleteResolvedRecord(save: false, context: context)
                } else if action == SyncActions.create {
                    
                    if clientRecord != nil {
                        // This can happen pretty often, especially for records that don't use diffs (e.g. prefs>devices)
                        // They always return a create command, even if they already "exist", since there is no real 'resolving'
                        //  Hence check below to prevent duplication
                    }
                    
                    // TODO: Needs favicon
                    if clientRecord == nil {
                        _ = recordType.coredataModelType?.createResolvedRecord(rootObject: fetchedRoot, save: false, context: .existing(context))
                    } else {
                        // TODO: use Switch with `fallthrough`
                        action = .update
                    }
                }
                
                // Handled outside of else block since .create, can modify to an .update
                if action == .update {
                    clientRecord?.updateResolvedRecord(fetchedRoot, context: .existing(context))
                }
            }
        }
        log.debug("\(fetchedRecords.count) \(recordType.rawValue) processed")
        
        if fetchedRecords.isEmpty { return }
        
        // After records have been written, without issue, save timestamp
        // We increment by a single millisecond to make sure we don't re-fetch the same duplicate records over and over
        // If there are more records with the same timestamp than the batch size, they will be dropped,
        //  however this is unimportant, as it actually prevents an infinitely recursive loop, of refetching the same records over
        //  and over again
        if recordType == .bookmark {
            Preferences.Sync.lastFetchTimestamp.value += 1
            
            if self.lastFetchWasTrimmed {
                // Do fast refresh, do not wait for timer
                self.fetch(type: .bookmark)
                self.lastFetchWasTrimmed = false
            }
            
            syncFetchedHandlers.forEach { $0() }
        } else if recordType == .prefs {
            Preferences.Sync.lastPreferenceFetchTimestamp.value += 1
        }
    }
    
    func deleteSyncUser(_ data: [String: AnyObject]) {
        log.warning("not implemented: deleteSyncUser() \(data)")
    }
    
    func deleteSyncCategory(_ data: [String: AnyObject]) {
        log.warning("not implemented: deleteSyncCategory() \(data)")
    }
    
    func deleteSyncSiteSettings(_ data: [String: AnyObject]) {
        log.warning("not implemented: delete sync site settings \(data)")
    }
    
}

// MARK: Server To Native Message category
extension Sync {
    
    func getExistingObjects(_ data: SyncResponse?) {
        
        guard let recordJSON = data?.rootElements, let apiRecodType = data?.arg1, let recordType = SyncRecordType(rawValue: apiRecodType) else { return }
        
        guard let fetchedRecords = recordType.fetchedModelType?.syncRecords(recordJSON) else { return }
        
        let ids = fetchedRecords.map { $0.objectId }.compactMap { $0 }
        
        var localRecord: [Syncable]?
        
        DataController.perform { context in
            localRecord = recordType.coredataModelType?.get(syncUUIDs: ids, context: context) as? [Syncable]
        
	        var matchedRecords = [[Any]]()
	        let deviceId = Device.currentDevice(context: context)?.deviceId
	        
	        for fetchedRecord in fetchedRecords {
	            var local: Any = "null"
	            
	            if let found = localRecord?.find({ $0.syncUUID != nil && $0.syncUUID == fetchedRecord.objectId }) {
	                local = found.asDictionary(deviceId: deviceId, action: fetchedRecord.action)
	            }
	            
	            matchedRecords.append([fetchedRecord.dictionaryRepresentation(), local])
	        }
	        
	        // TODO: Check if parsing not required
	        guard let serializedData = JSONSerialization.jsObject(withNative: matchedRecords as AnyObject, escaped: false) else {
	            log.error("Critical error: could not serialize data for resolve-sync-records")
	            return
	        }
	        
            let lastFetchTimestamp = data?.lastFetchedTimestamp ?? 0
	        // Only currently support bookmarks, this data will be abstracted (see variable definition note)
	        if recordType == .bookmark {
                if lastFetchTimestamp > Preferences.Sync.lastFetchTimestamp.value {
                    Preferences.Sync.lastFetchTimestamp.value = lastFetchTimestamp
                    log.info("sync bookmark last timestamp \(lastFetchTimestamp)")
                }
	            self.lastFetchWasTrimmed = data?.isTruncated ?? false
            } else if recordType == .prefs {
                
                if lastFetchTimestamp > Preferences.Sync.lastPreferenceFetchTimestamp.value {
                    Preferences.Sync.lastPreferenceFetchTimestamp.value = lastFetchTimestamp
                    log.info("sync preference last timestamp \(lastFetchTimestamp)")
                }
            }
	        
            DispatchQueue.main.async {
                self.webView.evaluateJavaScript("callbackList['resolve-sync-records'](null, '\(recordType.rawValue)', \(serializedData))",
                    completionHandler: { (result, error) in })
            }
        }
    }
    
    // Only called when the server has info for client to save
    func saveInitData(_ data: JSON) {
        // Sync Seed
        if let seedJSON = data["arg1"].array {
            let seed = seedJSON.compactMap({ $0.int })
            
            // TODO: Move to constant
            if seed.count < Sync.seedByteLength {
                // Error
                return
            }
            
            syncSeed = "\(seed)"
            
        } else if syncSeed == nil {
            // Failure
            log.error("Seed expected.")
        }
        
        // Device Id
        if let deviceArray = data["arg2"].array, deviceArray.count > 0 {
            // TODO: Just don't set, if bad, allow sync to recover on next init
            Device.currentDevice()?.deviceId = deviceArray.map { $0.intValue }
            DataController.save(context: Device.currentDevice()?.managedObjectContext)
        } else if Device.currentDevice()?.deviceId == nil {
            log.error("Device Id expected!")
        }
        
    }
    
    func syncSetupError() {
        syncSetupFailureCallback?()
    }
    
    func getBookmarkOrder(previousOrder: String?, nextOrder: String?) -> String? {
        
        // Empty string as a parameter means next/previous bookmark doesn't exist
        let prev = previousOrder ?? ""
        let next = nextOrder ?? ""
        
        let getBookmarkOrderFunction = jsContext?.objectForKeyedSubscript("getBookmarkOrder")
        
        guard let value = getBookmarkOrderFunction?.call(withArguments: [prev, next]).toString() else {
            return nil
        }
        
        if Bookmark.isSyncOrderValid(value) {
            return value
        }
        
        return nil
    }
}

extension Sync: WKScriptMessageHandler {
    public func userContentController(_ userContentController: WKUserContentController, didReceive message: WKScriptMessage) {
        
        // JS execution must be on main thread
        
        log.debug("😎 \(message.name) \(message.body)")
        
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
            log.debug("---- Sync Debug: \(data)")
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
        case "sync-setup-error":
            self.syncSetupError()
        default:
            log.debug("\(messageName) not handled yet")
        }
        
        self.checkIsSyncReady()
    }
}

