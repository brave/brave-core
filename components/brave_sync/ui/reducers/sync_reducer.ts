/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constants
import { types } from '../constants/sync_types'

// Utils
import * as storage from '../storage'

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

      const newDevices = payload.devices.map((device: any) => ({
        name: device.name,
        id: device.device_id,
        lastActive: (new Date(device.last_active)).toString()
      }))

      state = {
        ...state,
        isSyncConfigured: payload.settings.sync_configured,
        thisDeviceName: payload.settings.this_device_name,
        devices: [ ...state.devices, ...newDevices ]
      }
      break

    case types.SYNC_ON_HAVE_SEED_FOR_QR_CODE:
      console.log('[BY SYNC] seed for QR code is', payload.seed)
      break

    case types.SYNC_ON_HAVE_SYNC_WORDS:
      console.log('[BY SYNC] sync words are', payload.syncWords)
      break

    case types.SYNC_ON_SETUP_NEW_TO_SYNC:
      console.warn('[CEZAR] own device sync requested!', payload.thisDeviceName)
      chrome.send('setupSyncNewToSync', [payload.thisDeviceName])
      break


    case types.SYNC_ON_REQUEST_QR_CODE:
      chrome.send('needSyncQRcode')
      break

    case types.SYNC_ON_REQUEST_SYNC_WORDS:
      chrome.send('needSyncWords')
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
