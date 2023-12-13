// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Types
import { GasFeeOption } from '../constants/types'

export const gasFeeOptions: GasFeeOption[] = [
  {
    id: 'slow',
    name: 'braveSwapSlow'
  },
  {
    id: 'average',
    name: 'braveSwapAverage'
  },
  {
    id: 'fast',
    name: 'braveSwapFast'
  }
]
