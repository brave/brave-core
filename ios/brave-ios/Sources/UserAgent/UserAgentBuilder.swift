// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import UIKit

public struct UserAgentBuilder {
  private let device: UIDevice
  private let os: OperatingSystemVersion
  private let useBraveUserAgent: Bool

  /// - parameter iOSVersion: iOS version of the created UA. Both desktop and mobile UA differ between iOS versions.
  public init(
    device: UIDevice = .current,
    iOSVersion: OperatingSystemVersion = ProcessInfo().operatingSystemVersion,
    useBraveUserAgent: Bool = FeatureList.kUseBraveUserAgent.enabled
  ) {
    self.device = device
    self.os = iOSVersion
    self.useBraveUserAgent = useBraveUserAgent
  }

  private var braveMajorVersion: String {
    guard
      let version = Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString")
        as? String,
      let majorVersion = version.components(separatedBy: ".").first
    else {
      return ""
    }
    return majorVersion
  }

  /// Creates Safari-like user agent.
  /// - parameter desktopMode: Wheter to use Mac's Safari UA or regular mobile iOS UA.
  /// The desktop UA is taken from iOS Safari `Request desktop website` feature.
  ///
  /// - returns: A proper user agent to use in WKWebView and url requests.
  public func build(desktopMode: Bool, maskBrave: Bool = false) -> String {

    if desktopMode { return desktopUA(maskBrave: maskBrave) }

    let version =
      (useBraveUserAgent && !maskBrave)
      ? "Brave/\(braveMajorVersion)" : "Version/\(safariVersion)"

    return """
      Mozilla/5.0 (\(cpuInfo)) \
      AppleWebKit/605.1.15 (KHTML, like Gecko) \
      \(version) \
      Mobile/15E148 \
      Safari/604.1
      """
  }

  private var osVersion: String {
    var version = "\(os.majorVersion)_\(os.minorVersion)"
    if os.patchVersion > 0 {
      version += "_\(os.patchVersion)"
    }
    return version
  }

  // These user agents are taken from iOS Safari in desktop mode and hardcoded.
  // These are not super precise because each iOS version can have slighly different desktop UA.
  // The only differences are with exact `Version/XX` and `MAC OS X 10_XX` numbers.
  private func desktopUA(maskBrave: Bool = false) -> String {
    let braveUA =
      (useBraveUserAgent && !maskBrave) ? "Brave/\(braveMajorVersion) " : ""

    return
      """
      Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) \
      AppleWebKit/605.1.15 (KHTML, like Gecko) \(braveUA)\
      Version/\(safariVersion) \
      Safari/605.1.15
      """
  }

  private var cpuInfo: String {
    var currentDevice = device.model
    // Only use first part of device name(so "iPod Touch" becomes "iPod")
    if let deviceNameFirstPart = currentDevice.split(separator: " ").first {
      currentDevice = String(deviceNameFirstPart)
    }

    let platform = device.userInterfaceIdiom == .pad ? "OS" : "iPhone OS"

    return "\(currentDevice); CPU \(platform) \(osVersion) like Mac OS X"
  }

  private var safariVersion: String {
    var version = "\(os.majorVersion).\(os.minorVersion)"
    if os.patchVersion > 0 {
      version += ".\(os.patchVersion)"
    }
    return version
  }
}
