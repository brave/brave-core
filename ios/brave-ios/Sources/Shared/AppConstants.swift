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
  
  public var dauServerChannelParam: String {
    switch self {
    case .release:
      return "release"
    case .beta:
      return "beta"
    case .dev, .debug:
      return "developer"
    case .enterprise:
      return "invalid"
    }
  }
}

public struct KVOConstants: Equatable {
  public var keyPath: String
  
  public init(keyPath: String) {
    self.keyPath = keyPath
  }
  
  public static let loading: Self = .init(keyPath: "loading")
  public static let estimatedProgress: Self = .init(keyPath: "estimatedProgress")
  public static let URL: Self = .init(keyPath: "URL")
  public static let title: Self = .init(keyPath: "title")
  public static let canGoBack: Self = .init(keyPath: "canGoBack")
  public static let canGoForward: Self = .init(keyPath: "canGoForward")
  public static let hasOnlySecureContent: Self = .init(keyPath: "hasOnlySecureContent")
  public static let serverTrust: Self = .init(keyPath: "serverTrust")
  public static let _sampledPageTopColor: Self = .init(keyPath: "_sampl\("edPageTopC")olor")
}

public struct AppConstants {
  public static let isRunningTest = NSClassFromString("XCTestCase") != nil

  /// Build Channel.
  public static var buildChannel: AppBuildChannel = .release

  public static let webServerPort: Int = {
    AppConstants.buildChannel.isPublic ? 6571 : Int.random(in: 6572..<6600)
  }()
}
