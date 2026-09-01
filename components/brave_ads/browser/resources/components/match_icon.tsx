/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

export function MatchIcon({ isMatch }: { isMatch: boolean }) {
  return isMatch
    ? <Icon className='icon-success' name='check-circle-filled' />
    : <Icon className='icon-error' name='close-circle-filled' />
}
