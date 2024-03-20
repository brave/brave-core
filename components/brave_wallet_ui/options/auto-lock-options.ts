// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { getLocale } from '../../common/locale'
import { AutoLockOption } from '../constants/types'

export const autoLockOptions: AutoLockOption[] = [
  {
    label: getLocale('braveWalletAutoLockDurationFiveMinutes'),
    minutes: 5
  },
  {
    label: getLocale('braveWalletAutoLockDurationTenMinutes'),
    minutes: 10
  },
  {
    label: getLocale('braveWalletAutoLockDurationOneHour'),
    minutes: 60
  },
  {
    label: getLocale('braveWalletAutoLockDurationThreeHours'),
    minutes: 180
  }
]
