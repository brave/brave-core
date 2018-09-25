/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

// Constants
import { types } from '../constants/sync_types'


// ACTIONS TRIGGERED BY THE BACK-END. NEEDS TEST
export const onShowSettings = (settings: any, devices: any) => {
  return action(types.SYNC_ON_SHOW_SETTINGS, { settings, devices })
}
export const onHaveSyncWords = (syncWords: any) => {
  return action(types.SYNC_ON_HAVE_SYNC_WORDS, syncWords)
}
export const onHaveSeedForQrCode = (seed: any) => {
  return action(types.SYNC_ON_HAVE_SEED_FOR_QR_CODE, seed)
}
export const onLogMessage = (message: any) => {
  return action(types.SYNC_ON_LOG_MESSAGE, message)
}

// DEFAULT ACTIONS
export const syncInit = () => {
  return action(types.SYNC_INIT)
}

export const setupSyncOwnDevice = (mainDeviceName: string) => {
  return action(types.SYNC_SETUP_OWN_DEVICE, { mainDeviceName })
}

export const setupSyncAnotherDevice = (syncWords: string, deviceName: string) => {
  return action(types.SYNC_SETUP_ANOTHER_DEVICE, { syncWords, deviceName })
}

export const requestSyncQRCode = () => {
  return action(types.SYNC_REQUEST_QR_CODE)
}

export const requestSyncWords = () => {
  return action(types.SYNC_REQUEST_SYNC_WORDS)
}

export const toggleOwnDeviceSyncing = (setImmediateSyncDevice: boolean) => {
  return action(types.SYNC_TOGGLE_OWN_DEVICE_SYNCING, { setImmediateSyncDevice })
}

export const enableBookmarksSync = (shouldEnable: boolean) => {
  return action(types.SYNC_BOOKMARKS_DATA, { shouldEnable })
}

export const enableSavedSiteSettingsSync = (shouldEnable: boolean) => {
  return action(types.SYNC_SAVED_SITE_SETTINGS, { shouldEnable })
}

export const enableBrowsingHistorySync = (shouldEnable: boolean) => {
  return action(types.SYNC_BROWSING_HISTORY, { shouldEnable })
}

export const removeDeviceSync = (deviceId: number) => {
  return action(types.SYNC_REMOVE_DEVICE, { deviceId })
}

export const syncReset = () => {
  return action(types.SYNC_RESET)
}
