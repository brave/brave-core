/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import getActions from '../../../brave_new_tab_ui/api/getActions'
import { types as topSitesTypes } from '../../../brave_new_tab_ui/constants/grid_sites_types'

describe('new tab data api tests', () => {
  describe('getActions', () => {
    it('can call an action from getActions', () => {
      expect(getActions().showTilesRemovedNotice(true)).toEqual({
        payload: { shouldShow: true },
        type: topSitesTypes.SHOW_TILES_REMOVED_NOTICE
      })
    })
  })
})
