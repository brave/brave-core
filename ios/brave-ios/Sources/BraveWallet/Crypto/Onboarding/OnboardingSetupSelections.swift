// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

enum OnboardingSetupOption {
  case new
  case restore
}

struct OnboardingSetupSelections {
  let setupOption: OnboardingSetupOption
  /// Networks user selected to show in onboarding
  let networks: [Selectable<BraveWallet.NetworkInfo>]
}
