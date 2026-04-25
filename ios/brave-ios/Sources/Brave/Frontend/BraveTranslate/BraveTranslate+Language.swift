// Copyright 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

extension Locale.Language {
  /// The translate language must have a region for Chinese (TW vs. CN).
  /// For Manipuri, it must have a script.
  /// For all other languages it is a two/three letter language code with no region or script (minimal identifier)
  var braveTranslateLanguageIdentifier: String {
    if let region = region, let languageCode = languageCode?.identifier(.alpha2),
      languageCode == "zh"
    {
      return "\(languageCode)-\(region)"
    } else if let languageCode = languageCode, let script = script,
      languageCode.identifier(.alpha2) == "mni"
    {
      return "\(languageCode)-\(script)"
    }

    return languageCode?.identifier(.alpha2) ?? languageCode?.identifier ?? minimalIdentifier
  }
}
