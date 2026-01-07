/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { whenStepRendered } from './use_step_transition'
import { WelcomeStep } from './welcome_step'
import { ImportStep } from './import_step'
import { AppearanceStep } from './appearance_step'

import { style } from './app.style'

type Step = 'welcome' | 'import' | 'appearance'
const steps: Step[] = ['welcome', 'import', 'appearance']

export function App() {
  const [stepIndex, setStepIndex] = React.useState(2)
  const currentStep = steps[stepIndex] ?? 'welcome'

  function startStepTransition(dir: 'forward' | 'backward') {
    const index =
      dir === 'forward'
        ? Math.min(steps.length - 1, stepIndex + 1)
        : Math.max(0, stepIndex - 1)

    if (index === stepIndex) {
      return
    }

    document.startViewTransition({
      update: () => {
        setStepIndex(index)
        return whenStepRendered()
      },
      types: [dir],
    })
  }

  function stepBack() {
    startStepTransition('backward')
  }

  function stepForward() {
    startStepTransition('forward')
  }

  function renderStep() {
    switch (currentStep) {
      case 'welcome':
        return <WelcomeStep onNext={stepForward} />
      case 'import':
        return (
          <ImportStep
            onNext={stepForward}
            onBack={stepBack}
          />
        )
      case 'appearance':
        return (
          <AppearanceStep
            onNext={stepForward}
            onBack={stepBack}
          />
        )
    }
  }

  return (
    <div data-css-scope={style.scope}>
      <div className='welcome-container'>{renderStep()}</div>
    </div>
  )
}
