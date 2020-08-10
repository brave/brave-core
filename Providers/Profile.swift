/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// IMPORTANT!: Please take into consideration when adding new imports to
// this file that it is utilized by external components besides the core
// application (i.e. App Extensions). Introducing new dependencies here
// may have unintended negative consequences for App Extensions such as
// increased startup times which may lead to termination by the OS.
import Shared
import Storage
import XCGLogger
import SwiftKeychainWrapper

// Import these dependencies ONLY for the main `Client` application target.
#if MOZ_TARGET_CLIENT
    import SwiftyJSON
#endif

private let log = Logger.syncLogger

public let ProfileRemoteTabsSyncDelay: TimeInterval = 0.1

class ProfileFileAccessor: FileAccessor {
    convenience init(profile: Profile) {
        self.init(localName: profile.localName())
    }

    init(localName: String) {
        let profileDirName = "profile.\(localName)"

        // Bug 1147262: First option is for device, second is for simulator.
        var rootPath: String
        let sharedContainerIdentifier = AppInfo.sharedContainerIdentifier
        if let url = FileManager.default.containerURL(forSecurityApplicationGroupIdentifier: sharedContainerIdentifier) {
            rootPath = url.path
        } else {
            log.error("Unable to find the shared container. Defaulting profile location to ~/Library/Application Support/ instead.")
            rootPath = (NSSearchPathForDirectoriesInDomains(.applicationSupportDirectory,
                                                            .userDomainMask, true)[0])
        }

        super.init(rootPath: URL(fileURLWithPath: rootPath).appendingPathComponent(profileDirName).path)
        
        // Create the "Downloads" folder in the documents directory if doesn't exist.
        FileManager.default.getOrCreateFolder(name: "Downloads", excludeFromBackups: true, location: .documentDirectory)
    }
}

/**
 * A Profile manages access to the user's data.
 */
protocol Profile: class {
    var prefs: Prefs { get }
    var searchEngines: SearchEngines { get }
    var files: FileAccessor { get }
    var logins: BrowserLogins & SyncableLogins & ResettableSyncStorage { get }
    var certStore: CertStore { get }
    var recentlyClosedTabs: ClosedTabsStore { get }
    var panelDataObservers: PanelDataObservers { get }

    var isShutdown: Bool { get }
    
    func shutdown()
    func reopen()

    // I got really weird EXC_BAD_ACCESS errors on a non-null reference when I made this a getter.
    // Similar to <http://stackoverflow.com/questions/26029317/exc-bad-access-when-indirectly-accessing-inherited-member-in-swift>.
    func localName() -> String
}

fileprivate let PrefKeyClientID = "PrefKeyClientID"
extension Profile {
    var clientID: String {
        let clientID: String
        if let id = prefs.stringForKey(PrefKeyClientID) {
            clientID = id
        } else {
            clientID = UUID().uuidString
            prefs.setString(clientID, forKey: PrefKeyClientID)
        }
        return clientID
    }
}

open class BrowserProfile: Profile {
    
    fileprivate let name: String
    fileprivate let keychain: KeychainWrapper
    var isShutdown = false

    internal let files: FileAccessor

    let loginsDB: BrowserDB

    private static var loginsKey: String? {
        let key = "sqlcipher.key.logins.db"
        let keychain = KeychainWrapper.sharedAppContainerKeychain
        keychain.ensureStringItemAccessibility(.afterFirstUnlock, forKey: key)
        if keychain.hasValue(forKey: key) {
            return keychain.string(forKey: key)
        }

        let Length: UInt = 256
        let secret = Bytes.generateRandomBytes(Length).base64EncodedString
        keychain.set(secret, forKey: key, withAccessibility: .afterFirstUnlock)
        return secret
    }

    /**
     * N.B., BrowserProfile is used from our extensions, often via a pattern like
     *
     *   BrowserProfile(…).foo.saveSomething(…)
     *
     * This can break if BrowserProfile's initializer does async work that
     * subsequently — and asynchronously — expects the profile to stick around:
     * see Bug 1218833. Be sure to only perform synchronous actions here.
     *
     * A SyncDelegate can be provided in this initializer, or once the profile is initialized.
     * However, if we provide it here, it's assumed that we're initializing it from the application,
     * and initialize the logins.db.
     */
    init(localName: String, clear: Bool = false) {
        log.debug("Initing profile \(localName) on thread \(Thread.current).")
        self.name = localName
        self.files = ProfileFileAccessor(localName: localName)
        self.keychain = KeychainWrapper.sharedAppContainerKeychain

        if clear {
            do {
                // Remove the contents of the directory…
                try self.files.removeFilesInDirectory()
                // …then remove the directory itself.
                try self.files.remove("")
            } catch {
                log.info("Cannot clear profile: \(error)")
            }
        }

        // If the profile dir doesn't exist yet, this is first run (for this profile). The check is made here
        // since the DB handles will create new DBs under the new profile folder.
        let isNewProfile = !files.exists("")

        // Set up our database handles.
        self.loginsDB = BrowserDB(filename: "logins.db", secretKey: BrowserProfile.loginsKey, schema: LoginsSchema(), files: files)

        if isNewProfile {
            log.info("New profile. Removing old account metadata.")
            prefs.clearAll()
        }

        // Always start by needing invalidation.
        // This is the same as self.history.setTopSitesNeedsInvalidation, but without the
        // side-effect of instantiating SQLiteHistory (and thus BrowserDB) on the main thread.
        prefs.setBool(false, forKey: PrefsKeys.keyTopSitesCacheIsValid)
    }

    func reopen() {
        log.debug("Reopening profile.")
        isShutdown = false
        
        loginsDB.reopenIfClosed()
    }

    func shutdown() {
        log.debug("Shutting down profile.")
        isShutdown = true
        
        loginsDB.forceClose()
    }
    
    deinit {
        log.debug("Deiniting profile \(self.localName()).")
    }

    func localName() -> String {
        return name
    }

    lazy var panelDataObservers: PanelDataObservers = {
        return PanelDataObservers(profile: self)
    }()

    lazy var searchEngines: SearchEngines = {
        return SearchEngines(files: self.files)
    }()

    func makePrefs() -> Prefs {
        return NSUserDefaultsPrefs(prefix: self.localName())
    }

    lazy var prefs: Prefs = {
        return self.makePrefs()
    }()

    lazy var certStore: CertStore = {
        return CertStore()
    }()

    lazy var recentlyClosedTabs: ClosedTabsStore = {
        return ClosedTabsStore(prefs: self.prefs)
    }()

    lazy var logins: BrowserLogins & SyncableLogins & ResettableSyncStorage = {
        return SQLiteLogins(db: self.loginsDB)
    }()
}
