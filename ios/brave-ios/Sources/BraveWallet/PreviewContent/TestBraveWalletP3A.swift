// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore

#if DEBUG

class TestBraveWalletP3A: BraveWalletBraveWalletP3A {
  var _reportOnboarding: ((_ onboardingAction: BraveWallet.OnboardingAction) -> Void)?
  func reportOnboardingAction(_ onboardingAction: BraveWallet.OnboardingAction) {
    _reportOnboarding?(onboardingAction)
  }
}

#endif
