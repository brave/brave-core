// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

/* eslint-disable @typescript-eslint/array-type */
import * as React from 'react'

// components
import StepsNavigation, { StepsNavigationProps } from '../../../../../components/desktop/steps-navigation/steps-navigation'

export enum OnboardingSteps {
  createPassword = 'create-password',
  explainRecoveryPhrase = 'explain-recovery-phrase',
  backupRecoveryPhrase = 'backup-recovery-phrase',
  verifyRecoveryPhrase = 'verify-recovery-phrase',
  complete = 'complete'
}

const STEPS: OnboardingSteps[] = [
  OnboardingSteps.createPassword,
  OnboardingSteps.explainRecoveryPhrase,
  OnboardingSteps.backupRecoveryPhrase,
  OnboardingSteps.verifyRecoveryPhrase,
  OnboardingSteps.complete
]

interface Props extends Omit<StepsNavigationProps<OnboardingSteps>, 'steps'> {}

export const OnboardingStepsNavigation = ({
  currentStep,
  preventSkipAhead,
  goBack
}: Props) => {
  return <StepsNavigation
    goBack={goBack}
    steps={STEPS}
    currentStep={currentStep}
    preventSkipAhead={preventSkipAhead}
  />
}

export default OnboardingStepsNavigation
