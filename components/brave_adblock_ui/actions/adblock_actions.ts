/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

// Constants
import { types } from '../constants/adblock_types'

export const getCustomFilters = () => action(types.ADBLOCK_GET_CUSTOM_FILTERS)

export const onGetCustomFilters = (customFilters: string) =>
  action(types.ADBLOCK_ON_GET_CUSTOM_FILTERS, {
    customFilters
  })

export const statsUpdated = () => action(types.ADBLOCK_STATS_UPDATED)

export const updateCustomFilters = (customFilters: string) =>
  action(types.ADBLOCK_UPDATE_CUSTOM_FILTERS, {
    customFilters
  })
