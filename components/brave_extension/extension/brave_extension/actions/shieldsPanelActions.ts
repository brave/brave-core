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

export const reportBrokenSite: actions.ReportBrokenSite = () => {
  return {
    type: types.REPORT_BROKEN_SITE
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

export const allowScriptOriginsOnce: actions.AllowScriptOriginsOnce = () => {
  return {
    type: types.ALLOW_SCRIPT_ORIGINS_ONCE
  }
}

/**
 * Set a given script resource state to be in the allowed/blocked list
 * @param {string} url - The resource URL
 * @param {boolean} maybeBlock - Whether or not the resource should be blocked
 */
export const setScriptBlockedCurrentState: actions.SetScriptBlockedCurrentState = (url) => {
  return {
    type: types.SET_SCRIPT_BLOCKED_ONCE_CURRENT_STATE,
    url
  }
}

/**
 * Set all child resources of a given hostname to be in the allowed/blocked list
 * @param {string} origin - The blocked resource hostname
 * @param {boolean} maybeBlock - Whether or not the resource should be blocked
 */
export const setGroupedScriptsBlockedCurrentState: actions.SetGroupedScriptsBlockedCurrentState = (origin, maybeBlock) => {
  return {
    type: types.SET_GROUPED_SCRIPTS_BLOCKED_ONCE_CURRENT_STATE,
    origin,
    maybeBlock
  }
}

/**
 * Set all resources in blocked/allowed state to be in the allowed/blocked list
 * @param {boolean} maybeBlock - Whether or not the resource should be blocked
 */
export const setAllScriptsBlockedCurrentState: actions.SetAllScriptsBlockedCurrentState = (maybeBlock) => {
  return {
    type: types.SET_ALL_SCRIPTS_BLOCKED_ONCE_CURRENT_STATE,
    maybeBlock
  }
}

/**
 * Set the final state to all resources so they could be stored persistently in
 * the blocked/allowed list
 */
export const setFinalScriptsBlockedState: actions.SetFinalScriptsBlockedState = () => {
  return {
    type: types.SET_FINAL_SCRIPTS_BLOCKED_ONCE_STATE
  }
}

export const setAdvancedViewFirstAccess: actions.SetAdvancedViewFirstAccess = () => {
  return {
    type: types.SET_ADVANCED_VIEW_FIRST_ACCESS
  }
}

export const shieldsReady: actions.ShieldsReady = () => {
  return {
    type: types.SHIELDS_READY
  }
}

export const generateClassIdStylesheet = (tabId: number, classes: string[], ids: string[]) => {
  return {
    type: types.GENERATE_CLASS_ID_STYLESHEET,
    tabId,
    classes,
    ids
  }
}

export const cosmeticFilterRuleExceptions = (tabId: number, exceptions: string[], scriptlet: string) => {
  return {
    type: types.COSMETIC_FILTER_RULE_EXCEPTIONS,
    tabId,
    exceptions,
    scriptlet
  }
}

export const contentScriptsLoaded: actions.ContentScriptsLoaded = (tabId: number, url: string) => {
  return {
    type: types.CONTENT_SCRIPTS_LOADED,
    tabId,
    url
  }
}
