/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { getString } from '../../lib/strings'
import { useNewsActions } from '../../context/news_context'

import { style } from './news_opt_in.style'

export function NewsOptIn() {
  const actions = useNewsActions()

  return (
    <div data-css-scope={style.scope}>
      <div className='graphic' />
      <h3>{getString(S.BRAVE_NEWS_INTRO_TITLE)}</h3>
      <div>
        <p>{getString(S.BRAVE_NEWS_INTRO_DESCRIPTION)}</p>
      </div>
      <div>
        <Button
          kind='filled'
          onClick={() => {
            actions.setIsOptedIn(true)
            actions.setShowOnNTP(true)
          }}
        >
          {getString(S.BRAVE_NEWS_OPT_IN_ACTION_LABEL)}
        </Button>
        <Button
          kind='plain-faint'
          onClick={() => actions.setShowOnNTP(false)}
        >
          {getString(S.BRAVE_NEWS_OPT_OUT_ACTION_LABEL)}
        </Button>
      </div>
    </div>
  )
}
