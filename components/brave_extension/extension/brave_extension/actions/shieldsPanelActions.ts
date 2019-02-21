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

export const httpsEverywhereToggled: actions.HttpsEverywhereToggled = (setting) => {
  return {
    type: types.HTTPS_EVERYWHERE_TOGGLED,
    setting
  }
}

export const blockJavaScript: actions.BlockJavaScript = (setting) => {
  return {
    type: types.JAVASCRIPT_TOGGLED,
    setting
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

export const allowScriptOriginsOnce: actions.AllowScriptOriginsOnce = (origins) => {
  return {
    type: types.ALLOW_SCRIPT_ORIGINS_ONCE,
    origins
  }
}

export const changeNoScriptSettings: actions.ChangeNoScriptSettings = (origin) => {
  return {
    type: types.CHANGE_NO_SCRIPT_SETTINGS,
    origin
  }
}

export const changeAllNoScriptSettings: actions.ChangeAllNoScriptSettings = (origin, shouldBlock) => {
  return {
    type: types.CHANGE_ALL_NO_SCRIPT_SETTINGS,
    origin,
    shouldBlock
  }
}
