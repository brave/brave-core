// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
// IMPORTANT!: Please take into consideration when adding new imports to
// this file that it is utilized by external components besides the core
// application (i.e. App Extensions). Introducing new dependencies here
// may have unintended negative consequences for App Extensions such as
// increased startup times which may lead to termination by the OS.
import Shared
import Storage
import os.log

/// A Profile manages access to the user's data.
public protocol Profile: AnyObject {
  var prefs: Prefs { get }
  var searchEngines: SearchEngines { get }
  var certStore: CertStore { get }

  var isShutdown: Bool { get }

  func shutdown()
  func reopen()

  // I got really weird EXC_BAD_ACCESS errors on a non-null reference when I made this a getter.
  // Similar to <http://stackoverflow.com/questions/26029317/exc-bad-access-when-indirectly-accessing-inherited-member-in-swift>.
  func localName() -> String
}

private let prefKeyClientID = "PrefKeyClientID"
extension Profile {
  var clientID: String {
    let clientID: String
    if let id = prefs.stringForKey(prefKeyClientID) {
      clientID = id
    } else {
      clientID = UUID().uuidString
      prefs.setString(clientID, forKey: prefKeyClientID)
    }
    return clientID
  }
}

open class BrowserProfile: Profile {

  fileprivate let name: String
  public var isShutdown = false

  /// N.B., BrowserProfile is used from our extensions, often via a pattern like
  ///
  ///   BrowserProfile(…).foo.saveSomething(…)
  ///
  /// This can break if BrowserProfile's initializer does async work that
  /// subsequently — and asynchronously — expects the profile to stick around:
  /// see Bug 1218833. Be sure to only perform synchronous actions here.
  ///
  /// A SyncDelegate can be provided in this initializer, or once the profile is initialized.
  /// However, if we provide it here, it's assumed that we're initializing it from the application,
  /// and initialize the logins.db.
  public init(localName: String) {
    Logger.module.debug("Initing profile \(localName) on thread \(Thread.current).")
    self.name = localName

    if let profilePath = FileManager.default.containerURL(
      forSecurityApplicationGroupIdentifier: AppInfo.sharedContainerIdentifier
    )?.appendingPathComponent("profile.\(localName)").path {

      // If the profile dir doesn't exist yet, this is first run (for this profile). The check is made here
      // since the DB handles will create new DBs under the new profile folder.
      if !FileManager.default.fileExists(atPath: profilePath) {
        Logger.module.info("New profile. Removing old account metadata.")
        prefs.clearAll()
      }
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
    return SearchEngines()
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
