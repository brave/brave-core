/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as types from '../constants/shieldsPanelTypes'

export function shieldsPanelDataUpdated (details) {
  return { type: types.SHIELDS_PANEL_DATA_UPDATED, details }
}

export function shieldsToggled (setting = 'allow') {
  return { type: types.SHIELDS_TOGGLED, setting }
}

export function adBlockToggled () {
  return { type: types.AD_BLOCK_TOGGLED }
}

export function trackingProtectionToggled () {
  return { type: types.TRACKING_PROTECTION_TOGGLED }
}

export function resourceBlocked (details) {
  return { type: types.RESOURCE_BLOCKED, details }
}

export function blockAdsTrackers (setting) {
  return { type: types.BLOCK_ADS_TRACKERS, setting }
}

export function controlsToggled (setting = true) {
  return { type: types.CONTROLS_TOGGLED, setting }
}

export function httpsEverywhereToggled () {
  return { type: types.HTTPS_EVERYWHERE_TOGGLED }
}

export function javascriptToggled () {
  return { type: types.JAVASCRIPT_TOGGLED }
}
