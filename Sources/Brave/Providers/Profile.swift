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
import Foundation
import os.log

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
      var isDirectory: ObjCBool = false

      if !FileManager.default.fileExists(atPath: url.path, isDirectory: &isDirectory) {
        do {
          try FileManager.default.createDirectory(at: url, withIntermediateDirectories: true)
        } catch {
          Logger.module.error("Unable to find the shared container directory and error while trying tpo create a new directory. ")
        }
      }
      
      rootPath = url.path
    } else {
      Logger.module.error("Unable to find the shared container. Defaulting profile location to ~/Library/Application Support/ instead.")
      rootPath =
        (NSSearchPathForDirectoriesInDomains(
          .applicationSupportDirectory,
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
public protocol Profile: AnyObject {
  var prefs: Prefs { get }
  var searchEngines: SearchEngines { get }
  var files: FileAccessor { get }
  var certStore: CertStore { get }

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
  public var isShutdown = false

  public  let files: FileAccessor

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
  public init(localName: String, clear: Bool = false) {
    Logger.module.debug("Initing profile \(localName) on thread \(Thread.current).")
    self.name = localName
    self.files = ProfileFileAccessor(localName: localName)

    if clear {
      do {
        // Remove the contents of the directory…
        try self.files.removeFilesInDirectory()
        // …then remove the directory itself.
        try self.files.remove("")
      } catch {
        Logger.module.info("Cannot clear profile: \(error.localizedDescription)")
      }
    }

    // If the profile dir doesn't exist yet, this is first run (for this profile). The check is made here
    // since the DB handles will create new DBs under the new profile folder.
    let isNewProfile = !files.exists("")

    if isNewProfile {
      Logger.module.info("New profile. Removing old account metadata.")
      prefs.clearAll()
    }
  }

  public func reopen() {
    Logger.module.debug("Reopening profile.")
    isShutdown = false
  }

  public func shutdown() {
    Logger.module.debug("Shutting down profile.")
    isShutdown = true
  }

  deinit {
    Logger.module.debug("Deiniting profile \(self.localName()).")
  }

  public func localName() -> String {
    return name
  }

  public lazy var searchEngines: SearchEngines = {
    return SearchEngines(files: self.files)
  }()

  func makePrefs() -> Prefs {
    return NSUserDefaultsPrefs(prefix: self.localName())
  }

  public lazy var prefs: Prefs = {
    return self.makePrefs()
  }()

  public lazy var certStore: CertStore = {
    return CertStore()
  }()
}
