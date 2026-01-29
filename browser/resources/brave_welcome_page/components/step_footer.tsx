/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { getString } from '../lib/strings'

import { style } from './step_footer.style'

interface Props {
  onBack?: () => void
  onSkip?: () => void
  onAction: () => void
  actionText?: string
}

export function StepFooter(props: Props) {
  return (
    <div data-css-scope={style.scope}>
      <div className='back'>
        {props.onBack && (
          <Button
            kind='plain-faint'
            size='large'
            onClick={props.onBack}
          >
            {getString('WELCOME_PAGE_BACK_BUTTON_LABEL')}
          </Button>
        )}
      </div>
      <div className='forward'>
        {props.onSkip && (
          <Button
            kind='plain-faint'
            size='large'
            onClick={props.onSkip}
          >
            {getString('WELCOME_PAGE_SKIP_BUTTON_LABEL')}
          </Button>
        )}
        <Button
          className='main-button'
          kind='filled'
          size='large'
          onClick={props.onAction}
        >
          {props.actionText ?? getString('WELCOME_PAGE_NEXT_BUTTON_LABEL')}
        </Button>
      </div>
    </div>
  )
}
