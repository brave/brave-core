/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { style } from './styles'
import { StepDefinition } from './types'
import {
  StepWelcomeContent, StepWelcomeFooter,
  StepImportDataContent, StepImportDataFooter, ImportDataProvider,
  StepMakeYoursContent, StepMakeYoursFooter,
  StepCompleteContent, StepCompleteFooter
} from './steps'

// Define your onboarding steps here
const steps: StepDefinition[] = [
  { id: 'welcome', Content: StepWelcomeContent, Footer: StepWelcomeFooter },
  { id: 'import-data', Content: StepImportDataContent, Footer: StepImportDataFooter },
  { id: 'make-yours', Content: StepMakeYoursContent, Footer: StepMakeYoursFooter },
  { id: 'complete', Content: StepCompleteContent, Footer: StepCompleteFooter },
]

type TransitionDirection = 'forward' | 'backward'
type TransitionState = 'idle' | 'exiting' | 'entering'

const TRANSITION_DURATION = 300 // ms

export function App() {
  const [currentStepIndex, setCurrentStepIndex] = React.useState(0)
  const [displayedContentIndex, setDisplayedContentIndex] = React.useState(0)
  const [transitionState, setTransitionState] = React.useState<TransitionState>('idle')
  const [direction, setDirection] = React.useState<TransitionDirection>('forward')
  const [isInitialLoad, setIsInitialLoad] = React.useState(true)

  // Clear initial load flag after entrance animation completes
  React.useEffect(() => {
    const timer = setTimeout(() => {
      setIsInitialLoad(false)
    }, 1500) // 0.6s delay + 0.9s animation duration
    return () => clearTimeout(timer)
  }, [])

  const navigateToStep = React.useCallback((newIndex: number, dir: TransitionDirection) => {
    if (transitionState !== 'idle') return

    setDirection(dir)
    setTransitionState('exiting')

    setTimeout(() => {
      setDisplayedContentIndex(newIndex)
      setCurrentStepIndex(newIndex)
      setTransitionState('entering')

      setTimeout(() => {
        setTransitionState('idle')
      }, TRANSITION_DURATION)
    }, TRANSITION_DURATION)
  }, [transitionState])

  const handleNext = React.useCallback(() => {
    if (currentStepIndex < steps.length - 1) {
      navigateToStep(currentStepIndex + 1, 'forward')
    } else {
      // Last step completed - close welcome page or redirect
      console.log('Onboarding complete!')
    }
  }, [currentStepIndex, navigateToStep])

  const handleBack = React.useCallback(() => {
    if (currentStepIndex > 0) {
      navigateToStep(currentStepIndex - 1, 'backward')
    }
  }, [currentStepIndex, navigateToStep])

  const handleSkip = React.useCallback(() => {
    handleNext()
  }, [handleNext])

  // Keyboard navigation with arrow keys
  React.useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      // Don't navigate if user is typing in an input
      if (e.target instanceof HTMLInputElement || e.target instanceof HTMLTextAreaElement) {
        return
      }

      if (e.key === 'ArrowRight') {
        handleNext()
      } else if (e.key === 'ArrowLeft') {
        handleBack()
      }
    }

    window.addEventListener('keydown', handleKeyDown)
    return () => window.removeEventListener('keydown', handleKeyDown)
  }, [handleNext, handleBack])

  // Content uses displayedContentIndex (animated)
  const DisplayedContent = steps[displayedContentIndex].Content
  // Footer uses currentStepIndex (updates immediately, no animation)
  const CurrentFooter = steps[currentStepIndex].Footer

  const isFirstStep = currentStepIndex === 0
  const isLastStep = currentStepIndex === steps.length - 1

  const getTransitionClass = () => {
    if (transitionState === 'idle') return 'step-visible'
    if (transitionState === 'exiting') {
      return direction === 'forward' ? 'step-exit-left' : 'step-exit-right'
    }
    if (transitionState === 'entering') {
      return direction === 'forward' ? 'step-enter-from-right' : 'step-enter-from-left'
    }
    return ''
  }

  return (
    <ImportDataProvider>
      <div data-css-scope={style.scope}>
        <div className={`container ${isInitialLoad ? 'entrance-animation' : ''}`}>
          <div className="content-area">
            <div className="brave-logo-container">
              <Icon name='social-brave-release-favicon-fullheight-color'/>
            </div>
            <div className={`step-wrapper ${getTransitionClass()}`}>
              <DisplayedContent
                onNext={handleNext}
                onBack={handleBack}
                onSkip={handleSkip}
                isFirstStep={isFirstStep}
                isLastStep={isLastStep}
              />
            </div>
          </div>
          <CurrentFooter
            onNext={handleNext}
            onBack={handleBack}
            onSkip={handleSkip}
            isFirstStep={isFirstStep}
            isLastStep={isLastStep}
          />
        </div>
      </div>
    </ImportDataProvider>
  )
}
