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
 * @param {any} settings - the sync settings information
 * @param {any} devices - the sync devices information
*/
export const onShowSettings = (settings: any, devices: any) => {
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

/**
 * Dispatches a message telling the back-end that the user requested the QR code
 */
export const onRequestQRCode = () => {
  return action(types.SYNC_ON_REQUEST_QR_CODE)
}

/**
 * Dispatches a message telling the back-end that the user requested the sync words
 */
export const onRequestSyncWords = () => {
  return action(types.SYNC_ON_REQUEST_SYNC_WORDS)
}

/**
 * Dispatched by the back-end to inform useful log messages for debugging purposes
 * @param {string} message - the log message
 */
export const onLogMessage = (message: string) => {
  return action(types.SYNC_ON_LOG_MESSAGE, { message })
}