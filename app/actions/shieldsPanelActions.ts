/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as types from '../constants/shieldsPanelTypes'
import * as actions from '../types/actions/shieldsPanelActions'

export const shieldsPanelDataUpdated: actions.ShieldsPanelDataUpdated = (details) => {
  return {
    type: types.SHIELDS_PANEL_DATA_UPDATED,
    details
  }
}

export const shieldsToggled: actions.ShieldsToggled = (setting = 'allow') => {
  return {
    type: types.SHIELDS_TOGGLED,
    setting
  }
}

export const resourceBlocked: actions.ResourceBlocked = (details) => {
  return {
    type: types.RESOURCE_BLOCKED,
    details
  }
}

export const blockAdsTrackers: actions.BlockAdsTrackers = (setting) => {
  return {
    type: types.BLOCK_ADS_TRACKERS,
    setting
  }
}

export const controlsToggled: actions.ControlsToggled = (setting = true) => {
  return {
    type: types.CONTROLS_TOGGLED,
    setting
  }
}

export const httpsEverywhereToggled: actions.HttpsEverywhereToggled = () => {
  return {
    type: types.HTTPS_EVERYWHERE_TOGGLED
  }
}

export const javascriptToggled: actions.JavascriptToggled = () => {
  return {
    type: types.JAVASCRIPT_TOGGLED
  }
}

export const blockFingerprinting: actions.BlockFingerprinting = (setting) => {
  return {
    type: types.BLOCK_FINGERPRINTING,
    setting
  }
}

export const blockCookies: actions.BlockCookies = (setting) => {
  return {
    type: types.BLOCK_COOKIES,
    setting
  }
}
