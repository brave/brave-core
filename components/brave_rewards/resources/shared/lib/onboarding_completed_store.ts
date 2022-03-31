/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// An OnboardingCompletedStore can be used by apps that include
// rewards onboarding UX to store whether the user has interacted
// with the onboarding experience.
export class OnboardingCompletedStore {
  load () {
    return Boolean(localStorage.rewardsOnboardingComplete)
  }

  save () {
    localStorage.rewardsOnboardingComplete = String(Date.now())
  }
}
