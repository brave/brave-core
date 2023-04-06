/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

@testable import Brave
import Foundation
import Shared
import Storage
import XCTest

class MockFiles: FileAccessor {
  init() {
    let docPath = NSSearchPathForDirectoriesInDomains(.applicationSupportDirectory, .userDomainMask, true)[0]
    super.init(rootPath: (docPath as NSString).appendingPathComponent("testing"))
  }
}

open class MockProfile: Profile {

  // Read/Writeable properties for mocking
  public var files: FileAccessor

  fileprivate let name: String = "mockaccount"

  init() {
    files = MockFiles()
  }

  public func localName() -> String {
    return name
  }

  public func reopen() {
  }

  public func shutdown() {
  }

  public var isShutdown: Bool = false

  lazy public var isChinaEdition: Bool = {
    return Locale.current.identifier == "zh_CN"
  }()

  lazy public var certStore: CertStore = {
    return CertStore()
  }()

  lazy public var searchEngines: SearchEngines = {
    return SearchEngines(files: self.files)
  }()

  lazy public var prefs: Prefs = {
    return MockProfilePrefs()
  }()
}
