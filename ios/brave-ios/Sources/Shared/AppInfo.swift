// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

open class AppInfo {
  /// Return the main application bundle. If this is called from an extension, the containing app bundle is returned.
  public static var applicationBundle: Bundle {
    let bundle = Bundle.main
    switch bundle.bundleURL.pathExtension {
    case "app":
      return bundle
    case "appex":
      // .../Client.app/PlugIns/SendTo.appex
      return Bundle(url: bundle.bundleURL.deletingLastPathComponent().deletingLastPathComponent())!
    default:
      return Bundle.main
    }
  }

  public static var displayName: String {
    return applicationBundle.infoDictionaryString(forKey: "CFBundleDisplayName")
  }

  public static var appVersion: String {
    return applicationBundle.infoDictionaryString(forKey: "CFBundleShortVersionString")
  }

  public static var buildNumber: String {
    return applicationBundle.infoDictionaryString(forKey: String(kCFBundleVersionKey))
  }

  public static var majorAppVersion: String {
    return appVersion.components(separatedBy: ".").first!
  }

  /// Return the shared container identifier (also known as the app group) to be used with for example background
  /// http requests. It is the base bundle identifier with a "group." prefix.
  public static var sharedContainerIdentifier: String {
    var bundleIdentifier = baseBundleIdentifier

    // External beta
    if bundleIdentifier == "com.brave.ios.BrowserBeta" {
      // com.brave.ios.BrowserBeta is taken and can't be used as an app group.
      bundleIdentifier = "com.brave.ios.BrowserBeta.unique"
    }
    return "group." + bundleIdentifier
  }

  /// Return the keychain access group.
  public static func keychainAccessGroupWithPrefix(_ prefix: String) -> String {
    var bundleIdentifier = baseBundleIdentifier
    if bundleIdentifier == "com.brave.ios.FennecEnterprise" {
      // Bug 1373726 - Base bundle identifier incorrectly generated for Nightly builds
      // This can be removed when we are able to fix the app group in the developer portal
      bundleIdentifier = "com.brave.ios.Fennec.enterprise"
    }
    return prefix + "." + bundleIdentifier
  }

  /// Return the base bundle identifier.
  ///
  /// This function is smart enough to find out if it is being called from an extension or the main application. In
  /// case of the former, it will chop off the extension identifier from the bundle since that is a suffix not part
  /// of the *base* bundle identifier.
  public static var baseBundleIdentifier: String {
    let bundle = Bundle.main
    let packageType = bundle.infoDictionaryString(forKey: "CFBundlePackageType")
    let baseBundleIdentifier = bundle.bundleIdentifier!
    if packageType == "XPC!" {
      let components = baseBundleIdentifier.components(separatedBy: ".")
      return components[0..<components.count - 1].joined(separator: ".")
    }
    return baseBundleIdentifier
  }
}
