/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { style } from './empty_state.style'

interface Props {
  icon: string
  title: string
  text: string
}

export function EmptyState(props: Props) {
  return (
    <div data-css-scope={style.scope}>
      <div className='icon'>
        <Icon name={props.icon} />
      </div>
      <h3>{props.title}</h3>
      <p>{props.text}</p>
    </div>
  )
}
