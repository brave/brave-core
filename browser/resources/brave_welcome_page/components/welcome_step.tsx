/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { useWelcomeApi } from '../api/welcome_api_context'
import { useStepTransition } from './use_step_transition'
import { getString } from '../lib/strings'
import { StepHeader } from './step_header'

import { style } from './welcome_step.style'

interface Props {
  onNext: () => void
}

export function WelcomeStep(props: Props) {
  const api = useWelcomeApi()

  useStepTransition()

  return (
    <div
      data-css-scope={style.scope}
      className='step-view'
    >
      <div className='step-content'>
        <div className='step-text'>
          <StepHeader />
          <h1>{getString('WELCOME_PAGE_WELCOME_STEP_TITLE')}</h1>
          <p>{getString('WELCOME_PAGE_WELCOME_STEP_TEXT')}</p>
        </div>
        <div className='step-ui'>
          <div className='graphic' />
        </div>
      </div>
      <footer>
        <div className='forward'>
          <Button
            kind='plain-faint'
            size='large'
            onClick={props.onNext}
          >
            {getString('WELCOME_PAGE_MAYBE_LATER_BUTTON_LABEL')}
          </Button>
          <Button
            kind='filled'
            size='large'
            onClick={() => {
              api.setAsDefaultBrowser()
              props.onNext()
            }}
          >
            {getString('WELCOME_PAGE_SET_AS_DEFAULT_BUTTON_LABEL')}
          </Button>
        </div>
      </footer>
    </div>
  )
}
