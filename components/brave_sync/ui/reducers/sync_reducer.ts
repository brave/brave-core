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
    case types.SYNC_ON_SHOW_SETTINGS:
      console.log('settings', payload.settings)
      console.log('devices', payload.devices)
      state = {
        ...state,
        TBDsettings: payload.settings,
        TBDdevices: payload.devices
      }
      break
    case types.SYNC_ON_HAVE_SYNC_WORDS:
      console.log('syncWords', payload.syncWords)
      state = {
        ...state,
        TBDsyncWords: payload.syncWords
      }
      break
    case types.SYNC_ON_HAVE_SEED_FOR_QR_CODE:
      console.log('seed', payload.seed)
      state = {
        ...state,
        TBDseed: payload.seed
      }
      break
    case types.SYNC_ON_LOG_MESSAGE:
      console.log('message', payload.message)
      state = {
        ...state,
        TBDmessage: payload.message
      }
      break

    // inform SyncUIImpl we can request sync settings data
    case types.SYNC_INIT:
      // chrome.send('pageLoaded')
      break

    // setup own device syncing via "i am new to sync" button action
    case types.SYNC_SETUP_OWN_DEVICE:
      if (!payload.mainDeviceName) {
        break
      }
      chrome.send('setupSyncNewToSync', [payload.mainDeviceName])
      state = {
        ...state,
        isSyncEnabled: true,
        mainDeviceName: payload.mainDeviceName,
        devices: [
          ...state.devices, {
            deviceName: payload.mainDeviceName
          }
        ]
      }
      break

    // setup another device syncing via "i have an existing sync code" button action
    case types.SYNC_SETUP_ANOTHER_DEVICE:
      // console.log('state/ ', state)
      // chrome.send('setupSyncHaveCode', [payload.syncWords, payload.deviceName])
      // console.log([ ...state.devices, { deviceName: payload.deviceName } ])
      state = {
        ...state,
        isSyncEnabled: true,
        devices: [ ...state.devices, { deviceName: payload.deviceName } ]
      }
      break

    // inform the back-end that user requested a QR code
    case types.SYNC_REQUEST_QR_CODE:
      // chrome.send('needSyncQRcode')
      break

    // inform the back-end that user requested the sync words
    case types.SYNC_REQUEST_SYNC_WORDS:
      // chrome.send('needSyncWords')
      break

    // toggle whether or not current device is still syncing
    // this action is triggered from the enabled content panel
    case types.SYNC_TOGGLE_OWN_DEVICE_SYNCING:
      state = { ...state, setImmediateSyncDevice: payload.setImmediateSyncDevice }
      break

    // toggle enabled/disabled state about the sync of bookmarks data
    case types.SYNC_BOOKMARKS_DATA:
      state = { ...state, syncBookmarks: payload.shouldEnable }
      break

    // toggle enabled/disabled state about the sync of saved site settings
    case types.SYNC_SAVED_SITE_SETTINGS:
      state = { ...state, syncSavedSiteSettings: payload.shouldEnable }
      break

    // toggle enabled/disabled state about the sync of browsing history
    case types.SYNC_BROWSING_HISTORY:
      state = { ...state, syncBrowsingHistory: payload.shouldEnable }
      break

    // remove a given device from the sync chain
    case types.SYNC_REMOVE_DEVICE:
      // TODO
      // chrome.send('deleteDevice', action.deviceId)
      //   state = { ...state }
      //   break
      break

    // inform the back-end that sync will be reset
    case types.SYNC_RESET:
      // chrome.send('resetSync')
      state = { ...state, isSyncEnabled: false }
      break
  }

  if (state !== startingState) {
    storage.debouncedSave(state)
  }

  return state
}

export default syncReducer
