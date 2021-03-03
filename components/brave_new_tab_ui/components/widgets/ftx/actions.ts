// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { action } from 'typesafe-actions'
import { types } from './constants'

export const onFTXInteraction = () => action(types.ON_INTERACTION)

export const onFTXOptInMarkets = (show: boolean) => {
  return action(types.ON_MARKETS_OPT_IN, { show })
}
