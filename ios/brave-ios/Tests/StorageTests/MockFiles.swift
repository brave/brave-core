/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
@testable import Storage

import XCTest

class MockFiles: FileAccessor {
  init() {
    let docPath = NSSearchPathForDirectoriesInDomains(.applicationSupportDirectory, .userDomainMask, true)[0]
    super.init(rootPath: (docPath as NSString).appendingPathComponent("testing"))
  }
}

class SupportingFiles: FileAccessor {
  init() {
    let path = Bundle.main.bundlePath + "/PlugIns/StorageTests.xctest/"
    NSLog("Supporting files: \(path)")
    super.init(rootPath: path)
  }
}
