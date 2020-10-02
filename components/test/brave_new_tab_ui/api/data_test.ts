/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import getActions from '../../../brave_new_tab_ui/api/getActions'
import * as newTabActions from '../../../brave_new_tab_ui/actions/new_tab_actions'
import * as topSitesActions from '../../../brave_new_tab_ui/actions/grid_sites_actions'
import * as binanceActions from '../../../brave_new_tab_ui/actions/binance_actions'
import * as rewardsActions from '../../../brave_new_tab_ui/actions/rewards_actions'
import * as geminiActions from '../../../brave_new_tab_ui/actions/gemini_actions'
import * as bitcoinDotComActions from '../../../brave_new_tab_ui/actions/bitcoin_dot_com_actions'
import * as cryptoDotComActions from '../../../brave_new_tab_ui/actions/cryptoDotCom_actions'
import * as stackWidgetActions from '../../../brave_new_tab_ui/actions/stack_widget_actions'
import { types as topSitesTypes } from '../../../brave_new_tab_ui/constants/grid_sites_types'

describe('new tab data api tests', () => {
  describe('getActions', () => {
    it('returns an object with the same keys mimicking the original new tab actions', () => {
      const assertion = getActions()
      const actions = Object.assign({}, newTabActions, stackWidgetActions, topSitesActions, binanceActions, rewardsActions, geminiActions, bitcoinDotComActions, cryptoDotComActions)
      expect(Object.keys(assertion)).toEqual(Object.keys(actions))
    })
    it('can call an action from getActions', () => {
      expect(getActions().showTilesRemovedNotice(true)).toEqual({
        payload: { shouldShow: true },
        type: topSitesTypes.SHOW_TILES_REMOVED_NOTICE
      })
    })
  })
})
