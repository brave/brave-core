// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import CoreText

public class ReaderModeFonts {
  /// Registers the custom fonts that exist in the Brave bundle
  public static func registerCustomFonts() {
    if let ttfs = Bundle.module.urls(forResourcesWithExtension: "ttf", subdirectory: nil) {
      CTFontManagerRegisterFontURLs(ttfs as CFArray, .process, true, nil)
    }
    if let otfs = Bundle.module.urls(forResourcesWithExtension: "otf", subdirectory: nil) {
      CTFontManagerRegisterFontURLs(otfs as CFArray, .process, true, nil)
    }
  }
}
