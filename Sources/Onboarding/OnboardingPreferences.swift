// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveShared
import struct Shared.Strings
import BraveCore

extension Preferences {
  public final class Onboarding {
    /// Whether or not new user onboarding has completed.
    /// User skipping(tapping on skip) onboarding does NOT count as completed.
    /// If user kills the app before completing onboarding, it should be treated as unfinished.
    public static let basicOnboardingCompleted = Option<Int>(
      key: "general.basic-onboarding-completed",
      default: OnboardingState.undetermined.rawValue)
    /// The time until the next on-boarding shows
    public static let basicOnboardingDefaultBrowserSelected = Option<Bool>(
      key: "general.basic-onboarding-default-browser-selected",
      default: false)
    /// The progress the user has made with onboarding
    public static let basicOnboardingProgress = Option<Int>(key: "general.basic-onboarding-progress", default: OnboardingProgress.none.rawValue)
  }
}
