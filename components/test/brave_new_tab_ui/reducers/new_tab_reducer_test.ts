/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Reducer
import { newTabReducers } from '../../../brave_new_tab_ui/reducers'

// API
import * as storage from '../../../brave_new_tab_ui/storage/new_tab_storage'

describe('newTabReducer', () => {
  describe('initial state', () => {
    it('loads initial data', () => {
      const expectedState = storage.load()
      const returnedState = newTabReducers(undefined, { type: {} })
      expect(returnedState).toEqual(expectedState)
    })
  })

  describe('NEW_TAB_SET_INITIAL_DATA', () => {
    // TODO
  })
  describe('NEW_TAB_STATS_UPDATED', () => {
    // TODO
  })
  describe('NEW_TAB_DISMISS_BRANDED_WALLPAPER_NOTIFICATION', () => {
    // TODO
  })
  describe('NEW_TAB_PREFERENCES_UPDATED', () => {
    // TODO
  })
  describe('rewards features inside new tab page', () => {
    describe('CREATE_WALLET', () => {
      // TODO
    })
    describe('ON_ENABLED_MAIN', () => {
      // TODO
    })
    describe('CREATE_WALLET', () => {
      // TODO
    })
    describe('ON_ENABLED_MAIN', () => {
      // TODO
    })
    describe('ON_WALLET_INITIALIZED', () => {
      // TODO
    })
    describe('WALLET_CORRUPT', () => {
      // TODO
    })
    describe('WALLET_CREATED', () => {
      // TODO
    })
    describe('OK', () => {
      // TODO
    })
    describe('ON_ADS_ENABLED', () => {
      // TODO
    })
    describe('ON_ADS_ESTIMATED_EARNINGS', () => {
      // TODO
    })
    describe('ON_BALANCE_REPORT', () => {
      // TODO
    })
    describe('DISMISS_NOTIFICATION', () => {
      // TODO
    })
    describe('ON_PROMOTIONS', () => {
      // TODO
    })
    describe('ON_BALANCE', () => {
      // TODO
    })
    describe('ON_WALLET_EXISTS', () => {
      // TODO
    })
    describe('SET_PRE_INITIAL_REWARDS_DATA', () => {
      // TODO
    })
    describe('SET_INITIAL_REWARDS_DATA', () => {
      // TODO
    })
  })
})
