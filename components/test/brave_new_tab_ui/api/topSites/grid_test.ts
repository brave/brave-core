/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { getGridSites } from '../../../../brave_new_tab_ui/api/topSites/grid'

describe('new tab grid api tests', () => {
  const defaultState = {
    topSites: [],
    ignoredTopSites: [],
    pinnedTopSites: [],
    bookmarks: {}
  }
  describe('getGridSites', () => {
    it('allows http sites', () => {
      const url = 'http://cezaraugusto.net'
      const newState = {
        ...defaultState,
        topSites: [
          { url }
        ]
      }
      const assertion = getGridSites(newState, true)
      expect(assertion[0]).toBe(newState.topSites[0])
      expect(assertion[0].url).toBe(url)
    })
    it('allows https sites', () => {
      const url = 'https://cezaraugusto.net'
      const newState = {
        ...defaultState,
        topSites: [
          { url }
        ]
      }
      const assertion = getGridSites(newState, true)
      expect(assertion[0]).toBe(newState.topSites[0])
      expect(assertion[0].url).toBe(url)
    })
    it('do not allow the default chrome topSites url', () => {
      const url = 'https://chrome.google.com/webstore?hl=en'
      const newState = {
        ...defaultState,
        topSites: [
          { url }
        ]
      }
      const assertion = getGridSites(newState, true)
      expect(assertion[0]).toBe(undefined)
    })
  })
})
