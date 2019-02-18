/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as types from '../constants/cosmeticFilterTypes'
import * as actions from '../types/actions/cosmeticFilterActions'

export const siteCosmeticFilterAdded: actions.SiteCosmeticFilterAdded = (origin: string, cssfilter: string) => {
  return {
    type: types.SITE_COSMETIC_FILTER_ADDED,
    origin,
    cssfilter
  }
}

export const siteCosmeticFilterRemoved: actions.SiteCosmeticFilterRemoved = (origin: string) => {
  return {
    type: types.SITE_COSMETIC_FILTER_REMOVED,
    origin
  }
}

export const allCosmeticFiltersRemoved: actions.AllCosmeticFiltersRemoved = () => {
  return {
    type: types.ALL_COSMETIC_FILTERS_REMOVED
  }
}
