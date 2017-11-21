/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as shieldsPanelTypes from '../../constants/shieldsPanelTypes'
import {setAllowAdBlock, setAllowTrackingProtection} from '../api/shields'

const toggleValue = (value) => value === 'allow' ? 'block' : 'allow'

export default function shieldsPanelReducer (state = {}, action) {
  switch (action.type) {
    case shieldsPanelTypes.SHIELDS_PANEL_DATA_UPDATED:
      state = {...state, ...action.details}
      break
    case shieldsPanelTypes.TOGGLE_SHIELDS:
      setAllowAdBlock(state.origin, toggleValue(state.adBlock))
      setAllowTrackingProtection(state.origin, toggleValue(state.trackingProtection))
      break
    case shieldsPanelTypes.TOGGLE_AD_BLOCK:
      setAllowAdBlock(state.origin, toggleValue(state.adBlock))
      break
    case shieldsPanelTypes.TOGGLE_TRACKING_PROTECTION:
      setAllowTrackingProtection(state.origin, toggleValue(state.trackingProtection))
      break
  }
  return state
}
