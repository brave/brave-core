// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import Preferences

extension Preferences {
  /// Launch arguments can be set in scheme editor in Xcode.
  /// Note: these flags may be used for values in Debug scheme only, otherwise it returns nill when trying to access any of them.
  struct DebugFlag {
    private static let prefs = UserDefaults.standard

    static let skipOnboardingIntro = boolOrNil(for: "BRSkipOnboarding")
    static let skipEduPopups = boolOrNil(for: "BRSkipEduPopups")
    /// Skips default browser, Brave VPN and other in-app callouts.
    static let skipNTPCallouts = boolOrNil(for: "BRSkipAppLaunchPopups")

    private static func boolOrNil(for key: String) -> Bool? {
      if AppConstants.buildChannel != .debug || prefs.object(forKey: key) == nil {
        return nil
      }

      return prefs.bool(forKey: key)
    }
  }
}
