/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { getString } from '../../../lib/strings'

interface Props {
  title: string
  onBack: () => void
}

export function SubpageHeader(props: Props) {
  return (
    <h4 className='subpage-header'>
      <span>
        <Button
          size='small'
          kind='plain-faint'
          onClick={props.onBack}
        >
          <Icon
            slot='icon-before'
            name='carat-left'
            className='rotate-rtl'
          />
          {getString(S.BRAVE_NEWS_BACK_BUTTON)}
        </Button>
      </span>
      <span>{props.title}</span>
    </h4>
  )
}
