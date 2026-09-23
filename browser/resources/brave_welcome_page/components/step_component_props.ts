/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// The props used by all step components.
export interface StepComponentProps {
  // Handler for when the back button is pressed.
  onBack: () => void

  // Handler for when the next button is pressed.
  onNext: () => void

  // Whether this is the last step.
  isLastStep: boolean
}
