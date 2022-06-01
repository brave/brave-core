// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

/// A namespace for defining user facing copy
///
/// When making a new strings extension that defines strings using `NSLocalizedString`, ensure that you
/// add that swift file to the `l10n` framework target in Client.xcodeproj. The reason this needs to be done
/// is because Xcode's "Export Localization" feature does not pick up sources that exist in Swift Packages
/// so we must add these Swift files as targets of that framework.
///
/// Note that the `l10n` framework _does not_ need to compile.
public struct Strings {}

extension Bundle {
  /// The bundle where all localized strings are stored. When declaring a string using `NSLocalizedString`
  /// make sure you use this bundle.
  public static var strings: Bundle {
#if DEBUG
    if ProcessInfo.processInfo.environment["XCODE_RUNNING_FOR_PREVIEWS"] != "1" {
      return .module
    }
    
    let bundleName = "Brave_Strings"
    let candidates = [
      // Bundle should be present here when the package is linked into an App.
      Bundle.main.resourceURL,
      // Bundle should be present here when the package is linked into a framework.
      Bundle(for: BundleFinder.self).resourceURL,
      // For command-line tools.
      Bundle.main.bundleURL,
      // Bundle should be present in the build artifacts directory when linked into a SwiftUI Preview
      Bundle(for: BundleFinder.self).resourceURL?.deletingLastPathComponent(),
      Bundle(for: BundleFinder.self).resourceURL?.deletingLastPathComponent().deletingLastPathComponent()
    ]
    
    for candidate in candidates {
      let bundlePath = candidate?.appendingPathComponent("\(bundleName).bundle")
      if let bundle = bundlePath.flatMap(Bundle.init(url:)) {
        return bundle
      }
    }
    fatalError("unable to find bundle named \(bundleName)")
#else
    return .module
#endif
  }
  
  private class BundleFinder {}
}
