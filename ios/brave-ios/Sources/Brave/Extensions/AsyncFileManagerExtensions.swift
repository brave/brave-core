// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Foundation

extension AsyncFileManager {

  /// An explicit path to some piece of web data on disk
  struct WebDataPath {
    var value: String

    static let cookie: Self = .init(value: "Cookies")
    #if targetEnvironment(simulator)
    static let websiteData: Self = .init(
      value: "WebKit/\(Bundle.main.bundleIdentifier!)/WebsiteData"
    )
    #else
    static let websiteData: Self = .init(value: "WebKit/WebsiteData")
    #endif
  }

  /// Sets the folder access of a given WebDataPath
  func setWebDataAccess(
    atPath path: WebDataPath,
    lock: Bool
  ) async throws {
    let url = try url(for: .libraryDirectory, in: .userDomainMask).appending(path: path.value)
    try await setAttributes(
      [.posixPermissions: lock ? 0 : 0o755],
      ofItemAtPath: url.path(percentEncoded: false)
    )
  }

  /// Returns whether or a given WebDataPath is currently locked
  func isWebDataLocked(atPath path: WebDataPath) async throws -> Bool {
    let url = try url(for: .libraryDirectory, in: .userDomainMask).appending(path: path.value)
    let attributes = try await attributesOfItem(atPath: url.path(percentEncoded: false))
    if let posixPermissions = attributes[.posixPermissions] as? NSNumber {
      return posixPermissions == 0o755
    }
    return false
  }
}
