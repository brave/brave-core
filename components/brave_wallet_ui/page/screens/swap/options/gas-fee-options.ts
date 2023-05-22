// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { GasFeeOption } from '../constants/types'

// Assets
// FIXME(douglashdaniel): add the icons
// import { SlowIcon, AverageIcon, FastIcon } from '../assets/gas-presset-icons'

export const gasFeeOptions: GasFeeOption[] = [
  {
    id: 'slow',
    name: 'braveSwapSlow',
    icon: ''
  },
  {
    id: 'average',
    name: 'braveSwapAverage',
    icon: ''
  },
  {
    id: 'fast',
    name: 'braveSwapFast',
    icon: ''
  }
]
