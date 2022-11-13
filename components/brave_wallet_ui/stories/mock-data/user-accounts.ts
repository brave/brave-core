// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { UserAccountType } from '../../constants/types'

export const mockUserAccounts: UserAccountType[] = [
  {
    id: '1',
    name: 'Account 1',
    address: '0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14'
  },
  {
    id: '2',
    name: 'Account 2',
    address: '0x73A29A1da97149722eB09c526E4eAd698895bDCf'
  },
  {
    id: '3',
    name: 'Account 3',
    address: '0x3f29A1da97149722eB09c526E4eAd698895b426'
  }
]

export const mockRecoveryPhrase = [
  'tomato',
  'tomato',
  'velvet',
  'wishful',
  'span',
  'bowl',
  'atoms',
  'stone',
  'parent',
  'stop',
  'bowl',
  'exercise'
]

export const mockedMnemonic = mockRecoveryPhrase.join(' ')
