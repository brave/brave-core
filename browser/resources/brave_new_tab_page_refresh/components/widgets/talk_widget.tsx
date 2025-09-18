/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { useNewTabActions } from '../../context/new_tab_context'
import { WidgetMenu } from './widget_menu'
import { getString } from '../../lib/strings'
import { Link, openLink } from '../common/link'

import { style } from './talk_widget.style'

export function TalkWidget() {
  const actions = useNewTabActions()
  return (
    <div data-css-scope={style.scope}>
      <WidgetMenu>
        <leo-menu-item onClick={() => actions.setShowTalkWidget(false)}>
          <Icon name='eye-off' /> {getString('hideTalkWidgetLabel')}
        </leo-menu-item>
      </WidgetMenu>
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
          <Link url='https://brave.com/privacy/browser/#brave-talk-learn'>
            {getString('talkAboutDataLink')}
          </Link>
        </div>
      </div>
    </div>
  )
}
