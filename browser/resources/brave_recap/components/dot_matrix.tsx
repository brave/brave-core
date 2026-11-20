// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { style } from './dot_matrix.style'

// The page background: a grid of dots that swell where drifting clusters pass,
// with an orange highlight that sweeps across and tints them.
export function DotMatrix() {
  return (
    <div
      data-css-scope={style.scope}
      aria-hidden='true'
    >
      {['medium', 'large'].map((size) => (
        <div
          key={size}
          className={`dots ${size}`}
        >
          <div className='clusters'>
            <div className='shimmer' />
          </div>
        </div>
      ))}
    </div>
  )
}
