// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { AmountPresetObjectType } from '../constants/types'
import { getLocale } from '../../common/locale'

export const AmountPresetOptions = (): AmountPresetObjectType[] => [
  {
    name: getLocale('braveWalletPreset25'),
    value: 0.25
  },
  {
    name: getLocale('braveWalletPreset50'),
    value: 0.5
  },
  {
    name: getLocale('braveWalletPreset75'),
    value: 0.75
  },
  {
    name: getLocale('braveWalletPreset100'),
    value: 1
  }
]
