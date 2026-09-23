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
          <Icon name='eye-off' /> {getString(S.NEW_TAB_HIDE_TALK_WIDGET_LABEL)}
        </leo-menu-item>
      </WidgetMenu>
      <div className='title'>{getString(S.NEW_TAB_TALK_WIDGET_TITLE)}</div>
      <div className='content'>
        <div className='graphic' />
        <div className='text'>
          <div className='header'>
            {getString(S.NEW_TAB_TALK_DESCRIPTION_TITLE)}
          </div>
          <div>{getString(S.NEW_TAB_TALK_DESCRIPTION_TEXT)}</div>
        </div>
        <div className='actions'>
          <Button
            size='small'
            onClick={() => openLink('https://talk.brave.com/widget')}
          >
            {getString(S.NEW_TAB_TALK_START_CALL_LABEL)}
          </Button>
          <Link url='https://brave.com/privacy/browser/#brave-talk-learn'>
            {getString(S.NEW_TAB_TALK_ABOUT_DATA_LINK)}
          </Link>
        </div>
      </div>
    </div>
  )
}
