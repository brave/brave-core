/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { getString } from '../lib/strings'
import { StepFooter } from './step_footer'

import { style } from './import_step.style'

interface Props {
  onBack: () => void
  onNext: () => void
  onRender: () => void
}

export function ImportStep(props: Props) {
  React.useEffect(props.onRender, [props.onRender])

  return (
    <div
      data-css-scope={style.scope}
      className='step-view'
    >
      <div className='step-content'>
        <div className='step-text'>
          <h1>{getString('WELCOME_PAGE_IMPORT_STEP_TITLE')}</h1>
          <p>{getString('WELCOME_PAGE_IMPORT_STEP_TEXT1')}</p>
          <p>{getString('WELCOME_PAGE_IMPORT_STEP_TEXT2')}</p>
        </div>
        <div className='step-ui'></div>
      </div>
      <StepFooter
        onBack={props.onBack}
        onSkip={props.onNext}
        onAction={() => {}}
        actionText={getString('WELCOME_PAGE_IMPORT_BUTTON_LABEL')}
      />
    </div>
  )
}
