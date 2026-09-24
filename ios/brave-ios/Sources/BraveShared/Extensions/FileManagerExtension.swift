// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import UIKit
import os.log

extension FileManager {
  /// Returns size of contents of the directory in bytes.
  /// Returns nil if any error happened during the size calculation.
  @available(*, deprecated, message: "Use AsyncFileManager.sizeOfDirectory(at:)")
  public func directorySize(at directoryURL: URL) throws -> UInt64? {
    let allocatedSizeResourceKeys: Set<URLResourceKey> = [
      .isRegularFileKey,
      .fileAllocatedSizeKey,
      .totalFileAllocatedSizeKey,
    ]

    func fileSize(_ file: URL) throws -> UInt64 {
      let resourceValues = try file.resourceValues(forKeys: allocatedSizeResourceKeys)

      // We only look at regular files.
      guard resourceValues.isRegularFile ?? false else {
        return 0
      }

      // To get the file's size we first try the most comprehensive value in terms of what
      // the file may use on disk. This includes metadata, compression (on file system
      // level) and block size.
      // In case totalFileAllocatedSize is unavailable we use the fallback value (excluding
      // meta data and compression) This value should always be available.
      return UInt64(resourceValues.totalFileAllocatedSize ?? resourceValues.fileAllocatedSize ?? 0)
    }

    // We have to enumerate all directory contents, including subdirectories.
    guard
      let enumerator = self.enumerator(
        at: directoryURL,
        includingPropertiesForKeys: Array(allocatedSizeResourceKeys)
      )
    else { return nil }

    var totalSize: UInt64 = 0

    // Perform the traversal.
    for item in enumerator {
      // Add up individual file sizes.
      guard let contentItemURL = item as? URL else { continue }
      totalSize += try fileSize(contentItemURL)
    }

    return totalSize
  }
}
