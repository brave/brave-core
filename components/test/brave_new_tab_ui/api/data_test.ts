/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as dataFetchAPI from '../../../brave_new_tab_ui/api/dataFetch'
import * as actions from '../../../brave_new_tab_ui/actions/new_tab_actions'
import { types } from '../../../brave_new_tab_ui/constants/new_tab_types'

describe('new tab data api tests', () => {
  describe('getActions', () => {
    it('returns an object with the same keys mimicking the original new tab actions', () => {
      const assertion = dataFetchAPI.getActions()
      expect(Object.keys(assertion)).toEqual(Object.keys(actions))
    })
    it('can call an action from getActions', () => {
      const getActions = dataFetchAPI.getActions()
      expect(getActions.statsUpdated()).toEqual({
        meta: undefined,
        payload: undefined,
        type: types.NEW_TAB_STATS_UPDATED
      })
    })
  })

  describe('fetchTopSites', () => {
    let spy: jest.SpyInstance
    beforeEach(() => {
      spy = jest.spyOn(chrome.topSites, 'get')
    })
    afterEach(() => {
      spy.mockRestore()
    })
    it('calls chrome.topSites.get', () => {
      dataFetchAPI.fetchTopSites()
      expect(spy).toBeCalled()
    })
  })
})
