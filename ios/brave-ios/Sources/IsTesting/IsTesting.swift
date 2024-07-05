// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

/// Whether or not the current process is running tests.
///
/// https://github.com/pointfreeco/swift-issue-reporting/blob/main/Sources/IssueReporting/IsTesting.swift
public let isTesting = ProcessInfo.processInfo.isTesting

extension ProcessInfo {
  fileprivate var isTesting: Bool {
    if environment.keys.contains("XCTestBundlePath") { return true }
    if environment.keys.contains("XCTestConfigurationFilePath") { return true }
    if environment.keys.contains("XCTestSessionIdentifier") { return true }
    return arguments.contains { argument in
      let path = URL(fileURLWithPath: argument)
      return path.lastPathComponent == "xctest" || path.pathExtension == "xctest"
    }
  }
}
