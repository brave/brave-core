/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import ProgressRing from '@brave/leo/react/progressRing'

import { useLocaleContext } from '../../lib/locale_strings'

import { style } from './sending.style'

export function Sending() {
  const { getString } = useLocaleContext()
  return (
    <div {...style}>
      <ProgressRing />
      <h3>{getString('contributeSendingText')}</h3>
    </div>
  )
}
