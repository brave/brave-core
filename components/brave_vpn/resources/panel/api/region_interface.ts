// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
export interface Region {
  continent: string
  name: string
  namePretty: string
}
export interface RegionState {
  all?: Region[]
  current?: Region
  hasError: boolean
}
