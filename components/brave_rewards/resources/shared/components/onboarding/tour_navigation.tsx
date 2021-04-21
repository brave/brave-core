/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../lib/locale_context'
import { CaretIcon } from '../icons/caret_icon'

import * as style from './tour_navigation.style'

interface Props {
  stepCount: number
  currentStep: number
  firstTimeSetup: boolean
  layout?: 'narrow' | 'wide'
  postTourContent?: boolean
  onSelectStep: (step: number) => void
  onSkip: () => void
  onDone: () => void
}

export function TourNavigation (props: Props) {
  const { getString } = React.useContext(LocaleContext)

  if (props.currentStep < 0 ||
      props.currentStep >= props.stepCount ||
      props.stepCount <= 0) {
    return null
  }

  function stepCallback (step: number) {
    return () => {
      if (step !== props.currentStep &&
          step >= 0 &&
          step < props.stepCount) {
        props.onSelectStep(step)
      }
    }
  }

  const isFirst = props.currentStep === 0
  const isLast = props.currentStep === props.stepCount - 1

  if (isFirst) {
    const skipClick = () => props.onSkip()
    const skipButton = (
      <button className='nav-skip' onClick={skipClick}>
        {
          getString(props.firstTimeSetup && props.layout !== 'wide'
            ? 'onboardingTourSkipForNow'
            : 'onboardingTourSkip')
        }
      </button>
    )
    const startButton = (
      <button className='nav-forward nav-start' onClick={stepCallback(1)}>
        {getString('onboardingTourBegin')} <CaretIcon direction='right' />
      </button>
    )
    if (props.layout === 'wide') {
      return (
        <style.root>{skipButton}{startButton}</style.root>
      )
    }
    return (
      <style.root>
        <style.narrowStart>{startButton}{skipButton}</style.narrowStart>
      </style.root>
    )
  }

  const onBack = stepCallback(props.currentStep - 1)

  const onForward = isLast
    ? () => props.onDone()
    : stepCallback(props.currentStep + 1)

  const forwardContent = isLast
    ? getString(props.postTourContent
        ? 'onboardingTourContinue'
        : 'onboardingTourDone')
    : <>{getString('onboardingTourContinue')}<CaretIcon direction='right' /></>

  return (
    <style.root>
      <button className='nav-back' onClick={onBack}>
        <CaretIcon direction='left' />{getString('onboardingTourBack')}
      </button>
      <button className='nav-forward' onClick={onForward}>
        {forwardContent}
      </button>
    </style.root>
  )
}
