/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { load, defaultState } from './storage'

describe('local storage', () => {
  describe('loading', () => {
    it('returns default state if local storage data is not present', () => {
      expect(load()).toEqual(defaultState)
    })

    it('returns default state if local storage is not JSON', () => {
      localStorage['rewards-data'] = 'abc'
      expect(load()).toEqual(defaultState)
    })

    it('returns default state if local storage is not JSON object', () => {
      localStorage['rewards-data'] = '[]'
      expect(load()).toEqual(defaultState)
    })

    it('returns default state if local storage is empty JSON object', () => {
      localStorage['rewards-data'] = '{}'
      expect(load()).toEqual(defaultState)
    })

    it('returns default state if local storage version does not match', () => {
      localStorage['rewards-data'] = '{ "version": 0 }'
      expect(load()).toEqual(defaultState)
    })

    it('returns loaded state with initializing flag set', () => {
      const mockData = { version: defaultState.version }
      localStorage['rewards-data'] = JSON.stringify(mockData)
      expect(load()).toEqual({ ...mockData, initializing: true })
    })
  })
})
