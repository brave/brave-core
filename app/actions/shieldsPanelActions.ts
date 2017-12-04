/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as types from '../constants/shieldsPanelTypes'
import * as actions from '../types/actions/shieldsPanelActions'

const shieldsPanelDataUpdated: actions.shieldsPanelDataUpdated = (details: actions.details) => {
  return {
    type: types.SHIELDS_PANEL_DATA_UPDATED,
    details
  }
}

const shieldsToggled: actions.shieldsToggled = (setting: actions.settings = 'allow') => {
  return {
    type: types.SHIELDS_TOGGLED,
    setting
  }
}

const adBlockToggled: actions.adBlockToggled = () => {
  return {
    type: types.AD_BLOCK_TOGGLED
  }
}

const trackingProtectionToggled: actions.trackingProtectionToggled = () => {
  return {
    type: types.TRACKING_PROTECTION_TOGGLED
  }
}

const resourceBlocked: actions.resourceBlocked = (details: actions.details) => {
  return {
    type: types.RESOURCE_BLOCKED,
    details
  }
}

const blockAdsTrackers: actions.blockAdsTrackers = (setting: actions.settings) => {
  return {
    type: types.BLOCK_ADS_TRACKERS,
    setting
  }
}

const controlsToggled: actions.controlsToggled = (setting: boolean = true) => {
  return {
    type: types.CONTROLS_TOGGLED,
    setting
  }
}

const httpsEverywhereToggled: actions.httpsEverywhereToggled = () => {
  return {
    type: types.HTTPS_EVERYWHERE_TOGGLED
  }
}

const javascriptToggled: actions.javascriptToggled = () => {
  return {
    type: types.JAVASCRIPT_TOGGLED
  }
}

export default {
  shieldsPanelDataUpdated,
  shieldsToggled,
  adBlockToggled,
  trackingProtectionToggled,
  resourceBlocked,
  blockAdsTrackers,
  controlsToggled,
  httpsEverywhereToggled,
  javascriptToggled
}
