/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import getActions from '../../../brave_new_tab_ui/api/getActions'
import { getTopSites } from '../../../brave_new_tab_ui/api/topSites'
import * as actions from '../../../brave_new_tab_ui/actions/new_tab_actions'
import { types } from '../../../brave_new_tab_ui/constants/new_tab_types'

describe('new tab data api tests', () => {
  describe('getActions', () => {
    it('returns an object with the same keys mimicking the original new tab actions', () => {
      const assertion = getActions()
      expect(Object.keys(assertion)).toEqual(Object.keys(actions))
    })
    it('can call an action from getActions', () => {
      expect(getActions().onHideSiteRemovalNotification()).toEqual({
        payload: undefined,
        type: types.NEW_TAB_HIDE_SITE_REMOVAL_NOTIFICATION
      })
    })
  })

  describe('getTopSites', () => {
    let spy: jest.SpyInstance
    beforeEach(() => {
      spy = jest.spyOn(chrome.topSites, 'get')
    })
    afterEach(() => {
      spy.mockRestore()
    })
    it('calls chrome.topSites.get', async () => {
      await getTopSites()
      expect(spy).toBeCalled()
    })
  })
})
