/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

public enum AppBuildChannel: String {
  case release
  case beta
  case dev
  case enterprise
  case debug

  /// Whether this release channel is used/seen by external users (app store or testers)
  public var isPublic: Bool {
    // Using switch to force a return definition for each enum value
    // Simply using `return [.release, .beta].includes(self)` could lead to easily missing a definition
    //  if enum is ever expanded
    switch self {
    case .release, .beta:
      return true
    case .dev, .debug, .enterprise:
      return false
    }
  }

  public var serverChannelParam: String {
    switch self {
    case .release:
      return "release"
    case .beta:
      return "beta"
    case .dev:
      // This is designed to follow desktop platform
      return "developer"
    case .debug, .enterprise:
      return "invalid"
    }
  }
}

public enum KVOConstants: String {
  case loading = "loading"
  case estimatedProgress = "estimatedProgress"
  case URL = "URL"
  case title = "title"
  case canGoBack = "canGoBack"
  case canGoForward = "canGoForward"
  case hasOnlySecureContent = "hasOnlySecureContent"
  case serverTrust = "serverTrust"
}

public struct AppConstants {
  public static let isRunningTest = NSClassFromString("XCTestCase") != nil

  /// Build Channel.
  public static var buildChannel: AppBuildChannel = .release

  public static let webServerPort: Int = {
    AppConstants.buildChannel.isPublic ? 6571 : Int.random(in: 6572..<6600)
  }()
}
