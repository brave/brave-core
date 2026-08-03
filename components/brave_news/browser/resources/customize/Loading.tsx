/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import ProgressRing from '@brave/leo/react/progressRing'

import { style } from './Loading.style'

interface Props {
  // When true, fill a larger area suitable for the dialog Suspense fallback.
  fill?: boolean
}

export default function Loading(props: Props) {
  return (
    <div
      data-css-scope={style.scope}
      className={props.fill ? 'fill' : undefined}
    >
      <ProgressRing />
    </div>
  )
}
