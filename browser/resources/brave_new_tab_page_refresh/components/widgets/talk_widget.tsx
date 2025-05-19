/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { getString } from '../../lib/strings'
import { openLink } from '../common/link'

import { style } from './talk_widget.style'

export function TalkWidget() {
  return (
    <div data-css-scope={style.scope}>
      <div className='title'>
        {getString('talkWidgetTitle')}
      </div>
      <div className='content'>
        <div className='graphic' />
        <div className='text'>
          <div className='header'>
            {getString('talkDescriptionTitle')}
          </div>
          <div>
            {getString('talkDescriptionText')}
          </div>
        </div>
        <div className='actions'>
          <Button
            size='small'
            onClick={() => openLink('https://talk.brave.com/widget')}
          >
            {getString('talkStartCallLabel')}
          </Button>
        </div>
      </div>
    </div>
  )
}
