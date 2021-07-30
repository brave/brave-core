/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../lib/locale_context'

import { RewardsTourProps } from './rewards_tour_props'
import { TourNavigation } from './tour_navigation'
import { TourStepLinks } from './tour_step_links'
import { getTourPanels, getVerifyWalletPanel } from './rewards_tour_panels'

import * as style from './rewards_tour.style'

export function RewardsTour (props: RewardsTourProps) {
  const locale = React.useContext(LocaleContext)
  const [currentStep, setCurrentStep] = React.useState(0)
  const [showVerifyPanel, setShowVerifyPanel] = React.useState(false)
  const stepPanels = getTourPanels(props)
  const verifyPanel = getVerifyWalletPanel(locale, props)

  function getPanelNav () {
    const stepLinks = (
      <style.stepLinks>
        <TourStepLinks
          stepCount={stepPanels.length}
          currentStep={currentStep}
          onSelectStep={setCurrentStep}
        />
      </style.stepLinks>
    )

    const onNavSkip = () => {
      if (props.firstTimeSetup) {
        setCurrentStep(stepPanels.length - 1)
      } else {
        props.onDone()
      }
    }

    const onNavDone = () => {
      if (verifyPanel) {
        setShowVerifyPanel(true)
      } else {
        props.onDone()
      }
    }

    const tourNav = (
      <style.nav>
        <TourNavigation
          layout={props.layout}
          stepCount={stepPanels.length}
          currentStep={currentStep}
          firstTimeSetup={props.firstTimeSetup}
          postTourContent={Boolean(verifyPanel)}
          onSelectStep={setCurrentStep}
          onDone={onNavDone}
          onSkip={onNavSkip}
        />
      </style.nav>
    )

    return props.layout === 'wide'
      ? <>{tourNav}{stepLinks}</>
      : <>{stepLinks}{tourNav}</>
  }

  function getPanel () {
    if (showVerifyPanel) {
      return verifyPanel
    }
    if (stepPanels.length === 0 || currentStep >= stepPanels.length) {
      return null
    }
    return stepPanels[currentStep](locale, props)
  }

  const panel = getPanel()
  if (!panel) {
    return null
  }

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
        panel.actions
          ? <style.nav>{panel.actions}</style.nav>
          : getPanelNav()
      }
    </style.root>
  )
}
