/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

// Constants
import { types } from '../constants/adblock_types'

export const enableFilterList = (uuid: string, enabled: boolean) =>
  action(types.ADBLOCK_ENABLE_FILTER_LIST, {
    uuid,
    enabled
  })

export const getCustomFilters = () => action(types.ADBLOCK_GET_CUSTOM_FILTERS)

export const getRegionalLists = () => action(types.ADBLOCK_GET_REGIONAL_LISTS)

export const onGetCustomFilters = (customFilters: string) =>
  action(types.ADBLOCK_ON_GET_CUSTOM_FILTERS, {
    customFilters
  })

export const onGetRegionalLists = (regionalLists: AdBlock.FilterList[]) =>
  action(types.ADBLOCK_ON_GET_REGIONAL_LISTS, {
    regionalLists
  })

export const statsUpdated = () => action(types.ADBLOCK_STATS_UPDATED)

export const updateCustomFilters = (customFilters: string) =>
  action(types.ADBLOCK_UPDATE_CUSTOM_FILTERS, {
    customFilters
  })
