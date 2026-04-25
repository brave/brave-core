// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation

public enum OnboardingState: Int {
  /// Unknown, this can happen for upgrading users where other information
  /// (e.g. `firstLaunch` is used to determine the onboarding state.
  case undetermined
  /// The user has not seen this onboarding.
  case unseen
  /// The user has completed this onboarding.
  case completed
  /// The user has skipped the onboarding.
  case skipped
}

public enum OnboardingProgress: Int {
  /// The user has never started any onboarding.
  case none
  /// The user has seen new Tab Page
  case newTabPage
  /// The user has completed the rewards onboarding.
  case rewards
}
