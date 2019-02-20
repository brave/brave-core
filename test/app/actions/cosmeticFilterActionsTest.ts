/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as types from '../../../app/constants/cosmeticFilterTypes'
import * as actions from '../../../app/actions/cosmeticFilterActions'

describe('cosmeticFilterActions', () => {
  it('siteCosmeticFilterAdded action', () => {
    const origin = 'https://a.com'
    const cssfilter = '#filter'
    expect(actions.siteCosmeticFilterAdded(origin, cssfilter)).toEqual({
      type: types.SITE_COSMETIC_FILTER_ADDED,
      origin,
      cssfilter
    })
  })
  it('siteCosmeticFilterRemoved action', () => {
    const origin = 'https://a.com'
    expect(actions.siteCosmeticFilterRemoved(origin)).toEqual({
      type: types.SITE_COSMETIC_FILTER_REMOVED,
      origin
    })
  })
  it('allCosmeticFiltersRemoved action', () => {
    expect(actions.allCosmeticFiltersRemoved()).toEqual({
      type: types.ALL_COSMETIC_FILTERS_REMOVED
    })
  })
})
