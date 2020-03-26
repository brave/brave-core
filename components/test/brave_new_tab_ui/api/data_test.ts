/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import getActions from '../../../brave_new_tab_ui/api/getActions'
import { getTopSites } from '../../../brave_new_tab_ui/api/topSites'
import * as newTabActions from '../../../brave_new_tab_ui/actions/new_tab_actions'
import * as topSitesActions from '../../../brave_new_tab_ui/actions/grid_sites_actions'
import { types as topSitesTypes } from '../../../brave_new_tab_ui/constants/grid_sites_types'

describe('new tab data api tests', () => {
  describe('getActions', () => {
    it('returns an object with the same keys mimicking the original new tab actions', () => {
      const assertion = getActions()
      const actions = Object.assign({}, newTabActions, topSitesActions)
      expect(Object.keys(assertion)).toEqual(Object.keys(actions))
    })
    it('can call an action from getActions', () => {
      expect(getActions().showGridSiteRemovedNotification(true)).toEqual({
        payload: { shouldShow: true },
        type: topSitesTypes.GRID_SITES_SHOW_SITE_REMOVED_NOTIFICATION
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
