/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as types from '../constants/shieldsPanelTypes'

export function shieldsPanelDataUpdated (details) {
  return { type: types.SHIELDS_PANEL_DATA_UPDATED, details }
}

export function toggleShields () {
  return { type: types.TOGGLE_SHIELDS }
}

export function toggleAdBlock () {
  return { type: types.TOGGLE_AD_BLOCK }
}

export function toggleTrackingProtection () {
  return { type: types.TOGGLE_TRACKING_PROTECTION }
}

export function resourceBlocked (details) {
  return { type: types.RESOURCE_BLOCKED, details }
}
