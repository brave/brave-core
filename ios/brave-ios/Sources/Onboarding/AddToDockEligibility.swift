// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

/// Whether add-to-dock onboarding and Settings entry should be shown.
///
/// Scoped to English and US / Canada / Australia (non-EU English-speaking rollout per product).
public enum AddToDockEligibility {
  private static let allowedRegionIdentifiers: Set<String> = ["US", "CA", "AU"]

  public static var isEligible: Bool {
    isEligible(for: .autoupdatingCurrent)
  }

  /// Used by unit tests; production code should use `isEligible` (autoupdating locale).
  internal static func isEligible(for locale: Locale) -> Bool {
    guard locale.language.languageCode?.identifier == "en" else {
      return false
    }
    guard let region = locale.region?.identifier else {
      return false
    }
    return allowedRegionIdentifiers.contains(region)
  }
}
