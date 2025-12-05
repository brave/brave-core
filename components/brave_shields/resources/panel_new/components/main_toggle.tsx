/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { style } from './main_toggle.style'

interface Props {
  active: boolean
  disabled: boolean
  onToggle: (active: boolean) => void
}

export function MainToggle(props: Props) {
  return (
    <button
      data-css-scope={style.scope}
      onClick={() => props.onToggle(!props.active)}
      data-active={props.active}
      disabled={props.disabled}
    >
      <div className='thumb' />
      <div className='outer-border' />
    </button>
  )
}
