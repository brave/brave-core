/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'

// `isLoading` is for callers whose `isMatch` value is still an unloaded
// default (e.g. `false`) rather than a confirmed result; showing a spinner
// instead avoids a misleading fail/success icon while data is in flight.
export function MatchIcon(
  { isMatch, isLoading }: { isMatch: boolean, isLoading?: boolean },
) {
  if (isLoading) {
    return <ProgressRing className='match-icon-loading' />
  }
  return isMatch
    ? <Icon className='icon-success' name='check-circle-filled' />
    : <Icon className='icon-error' name='close-circle-filled' />
}
