// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import CoreText
import Foundation

public class FocusOnboardingFonts {
  /// Registers the custom fonts that exist in the Onboarding bundle
  public static func registerCustomFonts() {
    if let ttfs = Bundle.module.urls(forResourcesWithExtension: "ttf", subdirectory: nil) {
      CTFontManagerRegisterFontURLs(ttfs as CFArray, .process, true, nil)
    }
    if let otfs = Bundle.module.urls(forResourcesWithExtension: "otf", subdirectory: nil) {
      CTFontManagerRegisterFontURLs(otfs as CFArray, .process, true, nil)
    }
  }
}
