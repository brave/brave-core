/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Reducer
import { newTabReducer, newTabReducers } from '../../../brave_new_tab_ui/reducers'

// API
import * as storage from '../../../brave_new_tab_ui/storage/new_tab_storage'
import { types } from '../../../brave_new_tab_ui/constants/new_tab_types'

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
  describe('NEW_TAB_PRIVATE_TAB_DATA_UPDATED', () => {
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
    describe('LEDGER_OK', () => {
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
  describe('widget stack state maintenance', () => {
    describe('REMOVE_STACK_WIDGET', () => {
      it('does not modify an empty stack', () => {
        const assertion = newTabReducer({
          ...storage.defaultState,
          widgetStackOrder: []
        }, {
          type: types.REMOVE_STACK_WIDGET,
          payload: {
            widget: 'rewards'
          }
        })
        const expectedState = {
          ...storage.defaultState,
          widgetStackOrder: []
        }
        expect(assertion).toEqual(expectedState)
      })

      it('adds removed widget to removed widgets list', () => {
        const assertion = newTabReducer({
          ...storage.defaultState,
          removedStackWidgets: []
        }, {
          type: types.REMOVE_STACK_WIDGET,
          payload: {
            widget: 'rewards'
          }
        })
        const expectedState = {
          ...storage.defaultState,
          removedStackWidgets: ['rewards']
        }
        expect(assertion).toEqual(expectedState)
      })
    })
    describe('SET_FOREGROUND_STACK_WIDGET', () => {
      it('adds widget if it is not in the stack and sets it to the foreground', () => {
        const assertion = newTabReducer({
          ...storage.defaultState,
          widgetStackOrder: ['rewards']
        }, {
          type: types.SET_FOREGROUND_STACK_WIDGET,
          payload: {
            widget: 'binance'
          }
        })
        const expectedState = {
          ...storage.defaultState,
          widgetStackOrder: ['rewards', 'binance']
        }
        expect(assertion).toEqual(expectedState)
      })

      it('sets a widget to the foreground if it is in the stack', () => {
        const assertion = newTabReducer({
          ...storage.defaultState,
          widgetStackOrder: ['binance', 'rewards']
        }, {
          type: types.SET_FOREGROUND_STACK_WIDGET,
          payload: {
            widget: 'binance'
          }
        })
        const expectedState = {
          ...storage.defaultState,
          widgetStackOrder: ['rewards', 'binance']
        }
        expect(assertion).toEqual(expectedState)
      })

      it('does not re-add a widget', () => {
        const assertion = newTabReducer({
          ...storage.defaultState,
          widgetStackOrder: ['binance', 'rewards']
        }, {
          type: types.SET_FOREGROUND_STACK_WIDGET,
          payload: {
            widget: 'rewards'
          }
        })
        const expectedState = {
          ...storage.defaultState,
          widgetStackOrder: ['binance', 'rewards']
        }
        expect(assertion).toEqual(expectedState)
      })
    })
  })
})
