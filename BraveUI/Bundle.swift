// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

extension Bundle {
  /// Returns the resource bundle associated with the current Swift module.
  static var current: Bundle {
#if DEBUG
    if ProcessInfo.processInfo.environment["XCODE_RUNNING_FOR_PREVIEWS"] != "1" {
      return .module
    }
    
    let bundleName = "Brave_BraveUI"
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
    fatalError("unable to find bundle named Brave_BraveUI")
#else
    return .module
#endif
  }
  
  private class BundleFinder {}
  
}
