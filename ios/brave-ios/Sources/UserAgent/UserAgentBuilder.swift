// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import UIKit

public struct UserAgentBuilder {
  private let device: UIDevice
  private let os: OperatingSystemVersion
  private let useAppIdentifierEnabled: Bool
  private let appIdentifier: String

  /// - parameter iOSVersion: iOS version of the created UA. Both desktop and mobile UA differ between iOS versions.
  public init(
    device: UIDevice = .current,
    iOSVersion: OperatingSystemVersion = ProcessInfo().operatingSystemVersion,
    useAppIdentifierEnabled: Bool = FeatureList.kUseBraveUserAgent.enabled,
    appIdentifier: String = "Brave"
  ) {
    self.device = device
    self.os = iOSVersion
    self.useAppIdentifierEnabled = useAppIdentifierEnabled
    self.appIdentifier = appIdentifier
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

  /// Gets the current chromium version currently used
  /// - parameter majorVersionOnly: if it should only return the major version
  /// - returns: The chromium version currently used
  private func chromiumVersion(majorVersionOnly: Bool) -> String {
    if majorVersionOnly,
      let majorVersion = BraveCoreVersionInfo.chromiumVersion.components(separatedBy: ".").first
    {
      return majorVersion
    }
    return BraveCoreVersionInfo.chromiumVersion
  }

  /// Creates Safari-like user agent.
  /// - parameter desktopMode: Whether to use Mac's Safari UA or regular mobile iOS UA.
  /// - parameter useSafariUA: Whether to use Safari's UA or app identifier UA.
  /// - parameter useChromiumVersion: Whether to use chromium version or brave major version.
  /// The desktop UA is taken from iOS Safari `Request desktop website` feature.
  ///
  /// - returns: A proper user agent to use in WKWebView and url requests.
  public func build(
    desktopMode: Bool,
    useSafariUA: Bool = false,
    useChromiumVersion: Bool = false
  ) -> String {

    if desktopMode { return desktopUA(useSafariUA: useSafariUA) }

    let version =
      (useAppIdentifierEnabled && !useSafariUA)
      ? "\(appIdentifier)/\(useChromiumVersion ? chromiumVersion(majorVersionOnly: false) : braveMajorVersion)"
      : "Version/\(safariVersion)"

    return """
      Mozilla/5.0 (\(cpuInfo)) \
      AppleWebKit/605.1.15 (KHTML, like Gecko) \
      \(version) \
      Mobile/15E148 \
      Safari/604.1
      """
  }

  private var osVersion: String {
    if os.majorVersion >= 26 {
      // Starting with iOS/iPadOS 26, Safari freezes the os version in their
      // user agent string to the last version released before iOS 26.
      // We align with this to help protect against fingerprinting and for
      // improved webcompatibility.
      return "18_7"
    } else {
      var version = "\(os.majorVersion)_\(os.minorVersion)"
      if os.patchVersion > 0 {
        version += "_\(os.patchVersion)"
      }
      return version
    }
  }

  // These user agents are taken from iOS Safari in desktop mode and hardcoded.
  // These are not super precise because each iOS version can have slighly different desktop UA.
  // The only differences are with exact `Version/XX` and `MAC OS X 10_XX` numbers.
  private func desktopUA(
    useSafariUA: Bool = false,
    useChromiumVersion: Bool = false
  ) -> String {
    let braveUA =
      (useAppIdentifierEnabled && !useSafariUA)
      ? "\(appIdentifier)/\(useChromiumVersion ? chromiumVersion(majorVersionOnly: true) : braveMajorVersion) "
      : ""

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
