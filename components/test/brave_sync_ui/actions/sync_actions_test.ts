/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { types } from '../../../brave_sync/ui/constants/sync_types'
import * as actions from '../../../brave_sync/ui/actions/sync_actions'

describe('sync_actions', () => {
  it('onPageLoaded', () => {
    expect(actions.onPageLoaded()).toEqual({
      type: types.SYNC_ON_PAGE_LOADED,
      meta: undefined,
      payload: undefined
    })
  })

  it('onShowSettings', () => {
    const settings = {
      sync_bookmarks: true,
      sync_configured: true,
      sync_history: true,
      sync_settings: true,
      sync_this_device: true
    }
    const devices = {
      name: 'MIKE TYSON PC',
      device_id: 1,
      last_active: 0
    }
    expect(actions.onShowSettings(settings, devices)).toEqual({
      type: types.SYNC_ON_SHOW_SETTINGS,
      meta: undefined,
      payload: { settings, devices }
    })
  })

  it('onHaveSyncWords', () => {
    expect(actions.onHaveSyncWords('ameri do te karate')).toEqual({
      type: types.SYNC_ON_HAVE_SYNC_WORDS,
      meta: undefined,
      payload: { syncWords: 'ameri do te karate' }
    })
  })

  it('onHaveSeedForQrCode', () => {
    expect(actions.onHaveSeedForQrCode('seeeeeeeeeed')).toEqual({
      type: types.SYNC_ON_HAVE_SEED_FOR_QR_CODE,
      meta: undefined,
      payload: { seed: 'seeeeeeeeeed' }
    })
  })

  it('onSetupNewToSync', () => {
    expect(actions.onSetupNewToSync('BEST MACBOOK EVER')).toEqual({
      type: types.SYNC_ON_SETUP_NEW_TO_SYNC,
      meta: undefined,
      payload: { thisDeviceName: 'BEST MACBOOK EVER' }
    })
  })

  it('onRequestQRCode', () => {
    expect(actions.onRequestQRCode).toEqual(expect.any(Function))
  })

  it('onGenerateQRCodeImageSource', () => {
    expect(actions.onGenerateQRCodeImageSource('someImageConvertedToBase64')).toEqual({
      type: types.SYNC_ON_GENERATE_QR_CODE_IMAGE_SOURCE,
      meta: undefined,
      payload: { imageSource: 'someImageConvertedToBase64' }
    })
  })

  it('onRequestSyncWords', () => {
    expect(actions.onRequestSyncWords).toEqual(expect.any(Function))
  })

  it('onRemoveDevice', () => {
    expect(actions.onRemoveDevice(1337)).toEqual({
      type: types.SYNC_ON_REMOVE_DEVICE,
      meta: undefined,
      payload: { id: 1337 }
    })
  })

  it('onSyncReset', () => {
    expect(actions.onSyncReset()).toEqual({
      type: types.SYNC_ON_RESET,
      meta: undefined,
      payload: undefined
    })
  })

  it('onToggleSyncThisDevice', () => {
    expect(actions.onToggleSyncThisDevice(true)).toEqual({
      type: types.SYNC_ON_SYNC_THIS_DEVICE,
      meta: undefined,
      payload: { shouldSyncThisDevice: true }
    })
  })

  it('onSyncBookmarks', () => {
    expect(actions.onSyncBookmarks(true)).toEqual({
      type: types.SYNC_BOOKMARKS,
      meta: undefined,
      payload: { shouldEnable: true }
    })
  })

  it('onSyncSavedSiteSettings', () => {
    expect(actions.onSyncSavedSiteSettings(true)).toEqual({
      type: types.SYNC_SAVED_SITE_SETTINGS,
      meta: undefined,
      payload: { shouldEnable: true }
    })
  })

  it('onSyncBrowsingHistory', () => {
    expect(actions.onSyncBrowsingHistory(true)).toEqual({
      type: types.SYNC_BROWSING_HISTORY,
      meta: undefined,
      payload: { shouldEnable: true }
    })
  })

  it('onSetupSyncHaveCode', () => {
    expect(actions.onSetupSyncHaveCode('words...', 'macbook')).toEqual({
      type: types.SYNC_SETUP_SYNC_HAVE_CODE,
      meta: undefined,
      payload: { syncWords: 'words...', deviceName: 'macbook' }
    })
  })

  it('onLogMessage', () => {
    expect(actions.onLogMessage('hello from sync')).toEqual({
      type: types.SYNC_ON_LOG_MESSAGE,
      meta: undefined,
      payload: { message: 'hello from sync' }
    })
  })
})
