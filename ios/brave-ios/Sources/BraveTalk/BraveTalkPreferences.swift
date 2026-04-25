// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore

extension PrefService {
  /// Whether or not the Brave Talk feature in general is available to use and the UI should display
  /// buttons/settings for it.
  public var isBraveTalkAvailable: Bool {
    // Right now this feature is always available unless its managed/forced by policy
    let isDisabledByPolicy =
      isManagedPreference(forPath: kBraveTalkDisabledByPolicyPrefName)
      && boolean(forPath: kBraveTalkDisabledByPolicyPrefName)
    return !isDisabledByPolicy
  }
}
