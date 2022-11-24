/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadState } from './state_cache'
import { defaultState } from './default_state'

describe('local storage', () => {
  describe('loading', () => {
    it('returns default state if local storage data is not present', () => {
      expect(loadState()).toEqual(defaultState())
    })

    it('returns default state if local storage is not JSON', () => {
      localStorage['rewards-data'] = 'abc'
      expect(loadState()).toEqual(defaultState())
    })

    it('returns default state if local storage is not JSON object', () => {
      localStorage['rewards-data'] = '[]'
      expect(loadState()).toEqual(defaultState())
    })

    it('returns default state if local storage is empty JSON object', () => {
      localStorage['rewards-data'] = '{}'
      expect(loadState()).toEqual(defaultState())
    })
  })
})
