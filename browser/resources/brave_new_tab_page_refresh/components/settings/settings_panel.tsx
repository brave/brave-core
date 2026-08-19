/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { style } from './settings_panel.style'

interface Props {
  title: string
  cssScope: string
  onBack?: () => void
  children: React.ReactNode
}

// A titled group of settings displayed within the settings modal. Supply
// `onBack` when the panel was opened from another panel, in order to display
// the title as a back button.
export function SettingsPanel(props: Props) {
  return (
    <div data-css-scope={style.scope}>
      <h4>
        {props.onBack ? (
          <button
            className='title'
            onClick={props.onBack}
          >
            <Icon name='arrow-left' />
            {props.title}
          </button>
        ) : (
          props.title
        )}
      </h4>
      <div className='content'>
        <div data-css-scope={props.cssScope}>{props.children}</div>
      </div>
    </div>
  )
}
