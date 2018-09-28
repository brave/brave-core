/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constants
import { types } from '../constants/sync_types'

// Utils
import * as storage from '../storage'
import { generateQRCodeImageSource } from '../helpers'

const syncReducer: Reducer<Sync.State | undefined> = (state: Sync.State | undefined, action: any) => {
  if (state === undefined) {
    state = storage.load()
  }

  const payload = action.payload
  const startingState = state

  switch (action.type) {
    // inform SyncUIImpl we can start requesting sync settings data
    case types.SYNC_ON_PAGE_LOADED:
      console.warn('[CEZAR] page loaded fired...')
      chrome.send('pageLoaded')
      break

    case types.SYNC_ON_SHOW_SETTINGS:
      console.info('[SYNC] settings received:', payload.settings)
      console.info('[SYNC] devices received:', payload.devices)
      console.info('[SYNC] is bookmarks syncing?', payload.settings.sync_bookmarks)
      console.info('[SYNC] is sync configured?', payload.settings.sync_configured)
      console.info('[SYNC] is history syncing?', payload.settings.sync_history)
      console.info('[SYNC] is site settings syncing?', payload.settings.sync_settings)
      console.info('[SYNC] is this device syncing?', payload.settings.sync_this_device)

      const devices = payload.devices.map((device: Sync.DevicesFromBackEnd) => {
        return {
          name: device.name,
          id: device.device_id,
          lastActive: (new Date(device.last_active)).toString()
        }
      })

      state = {
        ...state,
        devices: [ ...devices ],
        isSyncConfigured: payload.settings.sync_configured,
        shouldSyncThisDevice: payload.settings.sync_this_device,
        thisDeviceName: payload.settings.this_device_name,
        syncBookmarks: payload.settings.sync_bookmarks,
        syncSavedSiteSettings: payload.settings.sync_settings,
        syncBrowsingHistory: payload.settings.sync_history
      }
      break

    case types.SYNC_ON_HAVE_SEED_FOR_QR_CODE:
      generateQRCodeImageSource(payload.seed)
      break

    case types.SYNC_ON_GENERATE_QR_CODE_IMAGE_SOURCE:
      state = { ...state, seedQRImageSource: payload.imageSource }
      break

    case types.SYNC_ON_HAVE_SYNC_WORDS:
      state = { ...state, syncWords: payload.syncWords }
      break

    case types.SYNC_ON_SETUP_NEW_TO_SYNC:
      chrome.send('setupSyncNewToSync', [payload.thisDeviceName])
      break


    case types.SYNC_ON_REQUEST_QR_CODE:
      chrome.send('needSyncQRcode')
      break

    case types.SYNC_ON_REQUEST_SYNC_WORDS:
      chrome.send('needSyncWords')
      break

    case types.SYNC_ON_RESET:
      chrome.send('resetSync')
      // sync is reset. clear all data
      state = { ...storage.defaultState }
      break

    case types.SYNC_ON_REMOVE_DEVICE:
      chrome.send('deleteDevice', [payload.id])
      break

    case types.SYNC_ON_SYNC_THIS_DEVICE:
      chrome.send('syncThisDevice', [payload.shouldSyncThisDevice])
      break

    case types.SYNC_BOOKMARKS:
      chrome.send('syncBookmarks', [payload.shouldEnable])
      break

    case types.SYNC_SAVED_SITE_SETTINGS:
      chrome.send('syncSavedSiteSettings', [payload.shouldEnable])
      break

    case types.SYNC_BROWSING_HISTORY:
      chrome.send('syncBrowsingHistory', [payload.shouldEnable])
      break

    case types.SYNC_SETUP_SYNC_HAVE_CODE:
      const wordsAsArray = payload.syncWords.split(' ')
      if (payload.deviceName.length === 0) {
        window.alert('device name is required')
      }
      if (wordsAsArray.length !== 24) {
        window.alert('Invalid input code')
        break
      }
      chrome.send('setupSyncHaveCode', [payload.syncWords, payload.deviceName])
      break

      case types.SYNC_ON_LOG_MESSAGE:
      if (process.env.TARGET_GEN_DIR !== 'prod') {
        console.info('[SYNC] log message received from sync:', payload.message)
      }
      break
  }

  if (state !== startingState) {
    storage.debouncedSave(state)
  }

  return state
}

export default syncReducer
