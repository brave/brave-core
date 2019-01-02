/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

// Constants
import { types } from '../constants/sync_types'

/**
 * Dispatches a message telling the back-end that sync page has loaded
 */
export const onPageLoaded = () => {
  return action(types.SYNC_ON_PAGE_LOADED)
}

/**
 * Action dispatched by the back-end with useful information
 * about devices and settings synced
 * @param {Sync.SettingsFromBackEnd} settings - the sync settings information
 * @param {Sync.DevicesFromBackEnd} devices - the sync original devices information
 */
export const onShowSettings = (settings: Sync.SettingsFromBackEnd, devices: Sync.DevicesFromBackEnd) => {
  return action(types.SYNC_ON_SHOW_SETTINGS, { settings, devices })
}

/**
 * Action dispatched by the back-end when sync words are available
 * for use in the front-end
 */
export const onHaveSyncWords = (syncWords: string) => {
  return action(types.SYNC_ON_HAVE_SYNC_WORDS, { syncWords })
}

/**
 * Action dispatched by the back-end containing the seed needed
 * to request the QR code
 * @param {string} seed - The seed used by the QR code generator
 */
export const onHaveSeedForQrCode = (seed: string) => {
  return action(types.SYNC_ON_HAVE_SEED_FOR_QR_CODE, { seed })
}

/**
 * Dispatches a message telling the back-end that a new device setup
 * was requested by the user
 * @param {string} thisDeviceName - The newly synced device name
 */
export const onSetupNewToSync = (thisDeviceName: string) => {
  return action(types.SYNC_ON_SETUP_NEW_TO_SYNC, { thisDeviceName })
}

export const onGenerateQRCodeImageSource = (imageSource: string) => {
  return action(types.SYNC_ON_GENERATE_QR_CODE_IMAGE_SOURCE, { imageSource })
}

/**
 * Dispatches a message telling the back-end that the user removed a given device
 * @param {number} id - The device ID
 * @param {string} deviceName - The device name
 */
export const onRemoveDevice = (id: number, deviceName: string) => {
  return action(types.SYNC_ON_REMOVE_DEVICE, { id, deviceName })
}

/**
 * Dispatches a message telling the back-end that user has reset Sync
 */
export const onSyncReset = () => {
  return action(types.SYNC_ON_RESET)
}

/**
 * Dispatches a message telling the back-end that user toggled the bookmarks sync
 * @param {boolean} shouldEnable - Whether or not it should sync the bookmarks
 */
export const onSyncBookmarks = (shouldEnable: boolean) => {
  return action(types.SYNC_BOOKMARKS, { shouldEnable })
}

/**
 * Dispatches a message telling the back-end that user toggled the site settings sync
 * @param {boolean} shouldEnable - Whether or not it should sync the site settings
 */
export const onSyncSavedSiteSettings = (shouldEnable: boolean) => {
  return action(types.SYNC_SAVED_SITE_SETTINGS, { shouldEnable })
}

/**
 * Dispatches a message telling the back-end that user toggled the browsing history sync
 * @param {boolean} shouldEnable - Whether or not it should sync browsing history
 */
export const onSyncBrowsingHistory = (shouldEnable: boolean) => {
  return action(types.SYNC_BROWSING_HISTORY, { shouldEnable })
}

/**
 * Setup another device by informing the sync words needed to connect.
 * Triggered by the initial Sync action "I have an existing Sync code"
 * @param {string} syncWords - The sync words needed to find the new device
 * @param {string} deviceName - The new device name
 */
export const onSetupSyncHaveCode = (syncWords: string, deviceName: string) => {
  return action(types.SYNC_SETUP_SYNC_HAVE_CODE, { syncWords, deviceName })
}

/**
 * Dispatched by the back-end when Sync encountered an error
 * @param {SetupErrorType} error - the error message
 */
export const onSyncSetupError = (error: Sync.SetupErrorType) => {
  return action(types.SYNC_SETUP_ERROR, { error })
}

/**
 * Reset the Sync Error
 */
export const clearSyncSetupError = () => {
  return action(types.SYNC_CLEAR_SETUP_ERROR)
}

/**
 * Dispatched by the back-end to inform useful log messages for debugging purposes
 * @param {string} message - the log message
 */
export const onLogMessage = (message: string) => {
  return action(types.SYNC_ON_LOG_MESSAGE, { message })
}
