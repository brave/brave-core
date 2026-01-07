/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

interface Props {
  backgroundColor: string
  foregroundColor: string
  baseColor: string
}

export function ThemeColorSwatch(props: Props) {
  return (
    <svg
      className='theme-color'
      viewBox='0 0 50 50'
      xmlns='http://www.w3.org/2000/svg'
    >
      <rect
        x='0'
        y='0'
        width='50'
        height='50'
        fill={props.foregroundColor}
      />
      <rect
        x='0'
        y='25'
        width='50'
        height='25'
        fill={props.backgroundColor}
      />
      <rect
        x='25'
        y='25'
        width='25'
        height='25'
        fill={props.baseColor}
      />
    </svg>
  )
}
