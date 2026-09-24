// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

extension AdblockService {
  /// The directory to which we should store all the dowloaded files into
  static var filterListBaseFolderURL: URL? {
    let location = FileManager.SearchPathDirectory.applicationSupportDirectory
    return FileManager.default.urls(for: location, in: .userDomainMask).first
  }

  /// Since the folder changes upon relaunches we cannot store the whole folder but the path.
  /// Here we re-construct the actual folder
  public static func makeAbsoluteURL(forComponentPath filePath: String?) -> URL? {
    // Combine the path with the base URL
    guard let filePath = filePath else { return nil }
    return filterListBaseFolderURL?.appendingPathComponent(filePath)
  }

  /// Since the folder changes upon relaunches we cannot store the whole folder but the path.
  /// Here we extract the path from the folder URL for storage
  public static func extractRelativePath(fromComponentURL filePath: URL?) -> String? {
    guard let baseURL = filterListBaseFolderURL, let filePath = filePath else {
      return nil
    }

    // We take the path after the base url
    if let range = filePath.path.range(of: [baseURL.path, "/"].joined()) {
      let folderPath = filePath.path[range.upperBound...]
      return String(folderPath)
    } else {
      assertionFailure(
        "You are either passing a componentURL that is invalid or somehow the base url changed in brave-core"
      )
      return nil
    }
  }
}
