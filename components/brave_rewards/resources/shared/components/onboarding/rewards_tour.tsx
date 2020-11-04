/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../lib/locale_context'

import { TourNavigation } from './tour_navigation'
import { TourStepLinks } from './tour_step_links'
import { getTourPanels } from './rewards_tour_panels'

import * as style from './rewards_tour.style'

interface Props {
  layout?: 'narrow' | 'wide'
  rewardsEnabled: boolean
  onDone: () => void
}

export function RewardsTour (props: Props) {
  const locale = React.useContext(LocaleContext)
  const [currentStep, setCurrentStep] = React.useState(0)
  const stepPanels = getTourPanels()

  if (stepPanels.length === 0 || currentStep >= stepPanels.length) {
    return null
  }

  const panel = stepPanels[currentStep](locale)

  const onSkip = () => {
    if (props.rewardsEnabled) {
      setCurrentStep(stepPanels.length - 1)
    } else {
      props.onDone()
    }
  }

  const stepLinks = (
    <style.stepLinks>
      <TourStepLinks
        stepCount={stepPanels.length}
        currentStep={currentStep}
        onSelectStep={setCurrentStep}
      />
    </style.stepLinks>
  )

  const tourNav = (
    <style.nav>
      <TourNavigation
        layout={props.layout}
        stepCount={stepPanels.length}
        currentStep={currentStep}
        rewardsEnabled={props.rewardsEnabled}
        onSelectStep={setCurrentStep}
        onDone={props.onDone}
        onSkip={onSkip}
      />
    </style.nav>
  )

  return (
    <style.root className={`tour-${props.layout || 'narrow'}`}>
      <style.stepHeader>{panel.heading}</style.stepHeader>
      <style.stepText>{panel.text}</style.stepText>
      <style.stepGraphic className={`tour-graphic-${panel.id}`} />
      {
        props.layout === 'wide'
          ? <>{tourNav}{stepLinks}</>
          : <>{stepLinks}{tourNav}</>
      }
    </style.root>
  )
}
