/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { OnboardingPhase } from '../api/welcome_api'
import { useWelcomeApi } from '../api/welcome_api_context'
import { Step, useStepList } from './use_step_list'
import { whenStepRendered } from './use_step_transition'
import { WelcomeStep } from './welcome_step'
import { ImportStep } from './import_step'
import { AppearanceStep } from './appearance_step'
import { FeaturesStep } from './features_step'
import { MetricsStep } from './metrics_step'

import { style } from './app.style'

// The onboarding phase reached by viewing each step. Steps that have no
// corresponding phase are not reported.
const stepPhases: Record<Step, OnboardingPhase | null> = {
  welcome: OnboardingPhase.kWelcome,
  import: OnboardingPhase.kImport,
  appearance: null,
  features: null,
  metrics: OnboardingPhase.kMetrics,
}

export function App() {
  const api = useWelcomeApi()
  const [stepIndex, setStepIndex] = React.useState(0)
  const steps = useStepList()
  const currentStep = steps[stepIndex] ?? 'welcome'

  React.useEffect(() => {
    const phase = stepPhases[currentStep]
    if (phase !== null) {
      api.setOnboardingPhase([phase])
    }
  }, [api, currentStep])

  if (steps.length === 0) {
    return (
      <div data-css-scope={style.scope}>
        <div className='loading' />
      </div>
    )
  }

  function startStepTransition(dir: 'forward' | 'backward') {
    const index =
      dir === 'forward'
        ? Math.min(steps.length - 1, stepIndex + 1)
        : Math.max(0, stepIndex - 1)

    if (index === stepIndex) {
      if (dir === 'forward') {
        api.setOnboardingPhase([OnboardingPhase.kFinished])
        api.getWelcomeCompleteURL.fetch().then((url) => {
          window.open(url, '_self', 'noopener')
        })
      }
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
    const isLastStep = stepIndex >= steps.length - 1

    switch (currentStep) {
      case 'welcome':
        return (
          <WelcomeStep
            onNext={stepForward}
            onBack={stepBack}
            isLastStep={isLastStep}
          />
        )
      case 'import':
        return (
          <ImportStep
            onNext={stepForward}
            onBack={stepBack}
            isLastStep={isLastStep}
          />
        )
      case 'appearance':
        return (
          <AppearanceStep
            onNext={stepForward}
            onBack={stepBack}
            isLastStep={isLastStep}
          />
        )
      case 'features':
        return (
          <FeaturesStep
            onNext={stepForward}
            onBack={stepBack}
            isLastStep={isLastStep}
          />
        )
      case 'metrics':
        return (
          <MetricsStep
            onNext={stepForward}
            onBack={stepBack}
            isLastStep={isLastStep}
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
