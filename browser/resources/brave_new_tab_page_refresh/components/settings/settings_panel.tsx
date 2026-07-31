/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { style } from './settings_panel.style'

interface Props {
  children: React.ReactNode
}

export function SettingsPanel(props: Props) {
  return (
    <div data-css-scope={style.scope}>
      <div className='content'>{props.children}</div>
    </div>
  )
}
