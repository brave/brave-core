/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import * as style from './tour_step_links.style'

interface Props {
  stepCount: number
  currentStep: number
  onSelectStep: (step: number) => void
}

export function TourStepLinks (props: Props) {
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

  return (
    <style.root>
      {
        Array(props.stepCount).fill(null).map((_, i) =>
          <button
            key={i}
            className={i === props.currentStep ? 'selected' : ''}
            onClick={stepCallback(i)}
          />
        )
      }
    </style.root>
  )
}
