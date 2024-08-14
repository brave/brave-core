// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

extension Locale {
  /// Regions focus onboarding is shown UK and Japan
  public var isNewOnboardingRegion: Bool {
    return Locale.current.region?.identifier == "JP" || Locale.current.region?.identifier == "GB"
  }
}
