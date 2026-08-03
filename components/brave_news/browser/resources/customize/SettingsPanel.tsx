/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { style } from './SettingsPanel.style'

interface Props {
  title: string
  cssScope: string
  children: React.ReactNode
}

// A titled group of settings displayed within the Brave News customize dialog.
export function SettingsPanel(props: Props) {
  return (
    <div data-css-scope={style.scope}>
      <h4>{props.title}</h4>
      <div className='content'>
        <div data-css-scope={props.cssScope}>{props.children}</div>
      </div>
    </div>
  )
}
