/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { getString } from '../lib/strings'
import { StepFooter } from './step_footer'

import { style } from './welcome_step.style'

interface Props {
  onNext: () => void
  onRender: () => void
}

export function WelcomeStep(props: Props) {
  React.useEffect(props.onRender, [props.onRender])

  return (
    <div
      data-css-scope={style.scope}
      className='step-view'
    >
      <div className='step-content'>
        <div className='step-text'>
          <h1>{getString('WELCOME_PAGE_WELCOME_STEP_TITLE')}</h1>
          <p>{getString('WELCOME_PAGE_WELCOME_STEP_TEXT')}</p>
        </div>
        <div className='step-ui'>
          <div className='graphic' />
        </div>
      </div>
      <StepFooter
        actionText={getString('WELCOME_PAGE_GET_STARTED_BUTTON_LABEL')}
        onAction={props.onNext}
      />
    </div>
  )
}
