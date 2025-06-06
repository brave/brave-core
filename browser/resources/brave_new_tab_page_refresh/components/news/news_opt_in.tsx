/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { getString } from '../../lib/strings'
import { useBraveNews } from '../../../../../components/brave_news/browser/resources/shared/Context'

import { style } from './news_opt_in.style'

export function NewsOptIn() {
  const { toggleBraveNewsOnNTP } = useBraveNews()

  return (
    <div data-css-scope={style.scope}>
      <div className='graphic' />
      <h3>{getString('newsEnableText')}</h3>
      <div>
        <p>{getString('newsOptInText')}</p>
      </div>
      <div>
        <Button
          kind='filled'
          onClick={() => toggleBraveNewsOnNTP(true)}
        >
          {getString('newsEnableButtonLabel')}
        </Button>
        <Button
          kind='plain-faint'
          onClick={() => toggleBraveNewsOnNTP(false)}
        >
          {getString('newsOptInDismissButtonLabel')}
        </Button>
      </div>
    </div>
  )
}
