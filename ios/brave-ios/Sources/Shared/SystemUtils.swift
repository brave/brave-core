// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

/// System helper methods written in Swift.
public struct SystemUtils {
  // This should be run on first run of the application.
  // It shouldn't be run from an extension.
  // Its function is to write a lock file that is only accessible from the application,
  // and not accessible from extension when the device is locked. Thus, we can tell if an extension is being run
  // when the device is locked.
  public static func onFirstRun() {
    guard let lockFileURL = lockedDeviceURL else {
      return
    }

    let lockFile = lockFileURL.path
    let fm = FileManager.default
    if fm.fileExists(atPath: lockFile) {
      return
    }
    let contents = "Device is unlocked".data(using: .utf8)
    fm.createFile(
      atPath: lockFile,
      contents: contents,
      attributes: [
        FileAttributeKey(rawValue: FileAttributeKey.protectionKey.rawValue): FileProtectionType
          .complete
      ]
    )
  }

  private static var lockedDeviceURL: URL? {
    let directoryURL = FileManager.default.containerURL(
      forSecurityApplicationGroupIdentifier: AppInfo.sharedContainerIdentifier
    )
    return directoryURL?.appendingPathComponent("security.dummy")
  }
}
