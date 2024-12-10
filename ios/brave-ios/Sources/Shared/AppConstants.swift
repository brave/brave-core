// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import UIKit

public enum AppBuildChannel: String {
  case release
  case beta
  case nightly
  case debug

  public var serverChannelParam: String {
    switch self {
    case .release:
      return "release"
    case .beta:
      return "beta"
    case .nightly:
      // This is designed to follow desktop platform
      return "nightly"
    case .debug:
      return "invalid"
    }
  }

  public var dauServerChannelParam: String {
    switch self {
    case .release:
      return "release"
    case .beta:
      return "beta"
    case .nightly, .debug:
      return "nightly"
    }
  }
}

public struct KVOConstants: Equatable {
  public var keyPath: String

  public init(keyPath: String) {
    self.keyPath = keyPath
  }

  // Safe for CWVWebView
  public static let loading: Self = .init(keyPath: "loading")
  public static let canGoBack: Self = .init(keyPath: "canGoBack")
  public static let canGoForward: Self = .init(keyPath: "canGoForward")
  public static let estimatedProgress: Self = .init(keyPath: "estimatedProgress")
  public static let visibleSSLStatus: Self = .init(keyPath: "visibleSSLStatus")
  public static let visibleURL: Self = .init(keyPath: "visibleURL")
  public static let title: Self = .init(keyPath: "title")
}

public struct AppConstants {
  public static let isRunningTest = NSClassFromString("XCTestCase") != nil

  /// Build Channel.
  public fileprivate(set) static var buildChannel: AppBuildChannel = .debug

  /// Whether or not this is an official build
  public fileprivate(set) static var isOfficialBuild: Bool = false
}

@_spi(AppLaunch)
extension AppConstants {
  public static func setBuildChannel(_ buildChannel: AppBuildChannel) {
    self.buildChannel = buildChannel
  }
  public static func setOfficialBuild(_ isOfficialBuild: Bool) {
    self.isOfficialBuild = isOfficialBuild
  }
}
