// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { ExpirationPresetObjectType } from '../constants/types'

export const ExpirationPresetOptions: ExpirationPresetObjectType[] = [
  {
    id: 1,
    name: '1D',
    expiration: 1
  },
  {
    id: 2,
    name: '3D',
    expiration: 3
  },
  {
    id: 3,
    name: '5D',
    expiration: 5
  },
  {
    id: 4,
    name: '1W',
    expiration: 7
  },
  {
    id: 5,
    name: '2W',
    expiration: 14
  }
]
