/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../lib/locale_context'

import { RewardsTourProps } from './rewards_tour_props'
import { TourNavigation } from './tour_navigation'
import { TourStepLinks } from './tour_step_links'
import { getTourPanels } from './rewards_tour_panels'

import * as style from './rewards_tour.style'

export function RewardsTour (props: RewardsTourProps) {
  const locale = React.useContext(LocaleContext)
  const [currentStep, setCurrentStep] = React.useState(0)
  const stepPanels = getTourPanels(props)

  if (stepPanels.length === 0 || currentStep >= stepPanels.length) {
    return null
  }

  const panel = stepPanels[currentStep](locale, props)

  const onSkip = () => {
    if (props.firstTimeSetup) {
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
        firstTimeSetup={props.firstTimeSetup}
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
      <style.stepContent>
        <style.stepGraphic className={`tour-graphic-${panel.id}`}>
          {panel.content}
        </style.stepGraphic>
      </style.stepContent>
      {
        props.layout === 'wide'
          ? <>{tourNav}{stepLinks}</>
          : <>{stepLinks}{tourNav}</>
      }
    </style.root>
  )
}
