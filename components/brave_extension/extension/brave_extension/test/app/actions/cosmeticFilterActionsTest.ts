/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as assert from 'assert'
import * as types from '../../../app/constants/cosmeticFilterTypes'
import * as actions from '../../../app/actions/cosmeticFilterActions'
import {} from '../../../app/types/actions/cosmeticFilterActions'

describe('cosmeticFilterActions', () => {
  it('siteCosmeticFilterAdded action', () => {
    const origin = 'https://a.com'
    const cssfilter = '#filter'
    assert.deepEqual(actions.siteCosmeticFilterAdded(origin, cssfilter), {
      type: types.SITE_COSMETIC_FILTER_ADDED,
      origin,
      cssfilter
    })
  })
  it('siteCosmeticFilterRemoved action', () => {
    const origin = 'https://a.com'
    assert.deepEqual(actions.siteCosmeticFilterRemoved(origin), {
      type: types.SITE_COSMETIC_FILTER_REMOVED,
      origin
    })
  })
  it('allCosmeticFiltersRemoved action', () => {
    assert.deepEqual(actions.allCosmeticFiltersRemoved(), {
      type: types.ALL_COSMETIC_FILTERS_REMOVED
    })
  })
})
