// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import os.log

struct LegacyLogsMigration {
  /// The old log system was based on XCGLogger with a rolling file saved to disk.
  /// This method removes old entries of that old log implementation.
  static func run() {
    let fileManager = FileManager.default
    
    guard let cacheDir = fileManager.urls(for: .cachesDirectory, in: .userDomainMask).first else {
      return
    }
    
    let logDir = cacheDir.appendingPathComponent("Logs")

    if !fileManager.fileExists(atPath: logDir.path) {
      return
    }

    do {
      try fileManager.removeItem(at: logDir)
    } catch {
      Logger.module.error("\(error.localizedDescription, privacy: .public)")
    }
  }
}
