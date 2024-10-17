// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Shared
import Storage
import TestHelpers
import UIKit
import WebKit
import XCTest

@testable import Brave

class ClientTests: XCTestCase {
  override func setUpWithError() throws {
    super.setUp()

    // TODO: Move this code into some module that is shared so it doesn't require AppDelegate/host application

    let responders: [(String, InternalSchemeResponse)] = [
      (AboutHomeHandler.path, AboutHomeHandler()),
      (AboutLicenseHandler.path, AboutLicenseHandler()),
      (SessionRestoreHandler.path, SessionRestoreHandler()),
      (ReaderModeHandler.path, ReaderModeHandler(profile: BrowserProfile(localName: "profile"))),
    ]

    responders.forEach { (path, responder) in
      InternalSchemeHandler.responders[path] = responder
    }
  }

  func testDownloadsFolder() async throws {
    let fileManager = AsyncFileManager.default
    let path = try await fileManager.downloadsPath()

    await XCTAssertAsyncTrue(await fileManager.fileExists(atPath: path.path))

    // Let's pretend user deletes downloads folder via files.app
    await XCTAssertAsyncNoThrow(try await fileManager.removeItem(at: path))

    await XCTAssertAsyncFalse(await fileManager.fileExists(atPath: path.path))

    // Calling downloads path should recreate the deleted folder
    await XCTAssertAsyncNoThrow(try await fileManager.downloadsPath())

    await XCTAssertAsyncTrue(await fileManager.fileExists(atPath: path.path))
  }
}
