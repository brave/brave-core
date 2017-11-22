/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as shieldsPanelTypes from '../../constants/shieldsPanelTypes'
import {setAllowAdBlock, setAllowTrackingProtection, toggleShieldsValue} from '../api/shields'
import {setBadgeText} from '../api/badge'

export default function shieldsPanelReducer (state = {tabs: {}}, action) {
  switch (action.type) {
    case shieldsPanelTypes.SHIELDS_PANEL_DATA_UPDATED:
      state = {...state, ...action.details}
      break
    case shieldsPanelTypes.TOGGLE_SHIELDS:
      setAllowAdBlock(state.origin, toggleShieldsValue(state.adBlock))
      setAllowTrackingProtection(state.origin, toggleShieldsValue(state.trackingProtection))
      break
    case shieldsPanelTypes.TOGGLE_AD_BLOCK:
      setAllowAdBlock(state.origin, toggleShieldsValue(state.adBlock))
      break
    case shieldsPanelTypes.TOGGLE_TRACKING_PROTECTION:
      setAllowTrackingProtection(state.origin, toggleShieldsValue(state.trackingProtection))
      break
    case shieldsPanelTypes.RESOURCE_BLOCKED:
      const tabId = action.details.tabId
      const tabs = {...state.tabs}
      tabs[tabId] = state.tabs[tabId] || { adsBlocked: 0, trackingProtectionBlocked: 0 }
      if (action.details.blockType === 'adBlock') {
        tabs[tabId].adsBlocked++
      } else if (action.details.blockType === 'trackingProtection') {
        tabs[tabId].trackingProtectionBlocked++
      }
      state = {
        ...state,
        tabs
      }
      if (tabId === state.tabId) {
        setBadgeText(state.tabs[tabId].adsBlocked + state.tabs[tabId].trackingProtectionBlocked)
      }
      break
  }
  return state
}
