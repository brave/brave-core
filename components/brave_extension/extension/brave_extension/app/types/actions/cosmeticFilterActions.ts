/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as types from '../constants/cosmeticFilterTypes'

interface SiteCosmeticFilterAddedReturn {
  type: types.SITE_COSMETIC_FILTER_ADDED,
  origin: string,
  cssfilter: string
}

export interface SiteCosmeticFilterAdded {
  (origin: string, cssfilter: string): SiteCosmeticFilterAddedReturn
}

interface SiteCosmeticFilterRemovedReturn {
  type: types.SITE_COSMETIC_FILTER_REMOVED,
  origin: string
}

export interface SiteCosmeticFilterRemoved {
  (origin: string): SiteCosmeticFilterRemovedReturn
}

interface AllCosmeticFiltersRemovedReturn {
  type: types.ALL_COSMETIC_FILTERS_REMOVED
}

export interface AllCosmeticFiltersRemoved {
  (): AllCosmeticFiltersRemovedReturn
}

export type cosmeticFilterActions =
  SiteCosmeticFilterRemovedReturn |
  SiteCosmeticFilterAddedReturn |
  AllCosmeticFiltersRemovedReturn
