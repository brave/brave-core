/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import XCTest

import Shared
import Storage
import WebKit
import BraveShared
@testable import Brave

class ClientTests: XCTestCase {
  override func setUpWithError() throws {
    super.setUp()
    
    // TODO: Move this code into some module that is shared so it doesn't require AppDelegate/host application
    
    let responders: [(String, InternalSchemeResponse)] = [
      (AboutHomeHandler.path, AboutHomeHandler()),
      (AboutLicenseHandler.path, AboutLicenseHandler()),
      (SessionRestoreHandler.path, SessionRestoreHandler()),
      (ErrorPageHandler.path, ErrorPageHandler()),
      (ReaderModeHandler.path, ReaderModeHandler(profile: BrowserProfile(localName: "profile")))
    ]
    
    responders.forEach { (path, responder) in
      InternalSchemeHandler.responders[path] = responder
    }
  }

  func testDownloadsFolder() {
    let path = try? FileManager.default.downloadsPath()
    XCTAssertNotNil(path)

    XCTAssert(FileManager.default.fileExists(atPath: path!.path))

    // Let's pretend user deletes downloads folder via files.app
    XCTAssertNoThrow(try FileManager.default.removeItem(at: path!))

    XCTAssertFalse(FileManager.default.fileExists(atPath: path!.path))

    // Calling downloads path should recreate the deleted folder
    XCTAssertNoThrow(try FileManager.default.downloadsPath())

    XCTAssert(FileManager.default.fileExists(atPath: path!.path))
  }
}
