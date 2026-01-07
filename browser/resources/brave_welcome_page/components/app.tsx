/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { WelcomeStep } from './welcome_step'
import { ImportStep } from './import_step'

import { style } from './app.style'

type Step = 'welcome' | 'import'
const steps: Step[] = ['welcome', 'import']

export function App() {
  const resolveTransitionRef = React.useRef<() => void>()
  const [stepIndex, setStepIndex] = React.useState(0)
  const currentStep = steps[stepIndex] ?? 'welcome'

  function startStepTransition(dir: 'forward' | 'back') {
    const index =
      dir === 'forward'
        ? Math.min(steps.length - 1, stepIndex + 1)
        : Math.max(0, stepIndex - 1)

    if (index === stepIndex) {
      return
    }

    document.startViewTransition({
      // @ts-expect-error: TS does not yet recognize `callbackOptions`
      update: () => {
        setStepIndex(index)
        return new Promise<void>((resolve) => {
          resolveTransitionRef.current = resolve
        })
      },
      types: [dir],
    })
  }

  function stepBack() {
    startStepTransition('back')
  }

  function stepForward() {
    startStepTransition('forward')
  }

  function onStepRendered() {
    if (resolveTransitionRef.current) {
      resolveTransitionRef.current()
    }
  }

  function renderStep() {
    switch (currentStep) {
      case 'welcome':
        return (
          <WelcomeStep
            onNext={stepForward}
            onRender={onStepRendered}
          />
        )
      case 'import':
        return (
          <ImportStep
            onNext={stepForward}
            onBack={stepBack}
            onRender={onStepRendered}
          />
        )
    }
  }

  return (
    <div data-css-scope={style.scope}>
      <div className='container'>
        <Icon
          name='brave-icon-release-color'
          className='logo'
        />
        {renderStep()}
      </div>
    </div>
  )
}
