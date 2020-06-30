/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { defaultState, migrateStackWidgetSettings, replaceStackWidgets } from '../../../brave_new_tab_ui/storage/new_tab_storage'

describe('new tab storage', () => {
  describe('migrateStackWidgetSettings', () => {
    it('migrates users who had all stack widgets hidden', () => {
      // showRewards and showBinance are false in default state
      // currentStackWidget will still be set in the event both
      // are hidden, as that did not drive visibility previously
      const initialState = {
        ...defaultState,
        currentStackWidget: 'rewards'
      }
      const expectedState = {
        ...defaultState,
        currentStackWidget: '',
        widgetStackOrder: [],
        removedStackWidgets: ['rewards', 'binance']
      }
      expect(migrateStackWidgetSettings(initialState)).toEqual(expectedState)
    })

    it('migrates users who were showing rewards and hiding binance', () => {
      const initialState = {
        ...defaultState,
        currentStackWidget: 'rewards',
        showRewards: true
      }
      const expectedState = {
        ...defaultState,
        showRewards: true,
        currentStackWidget: '',
        widgetStackOrder: ['rewards'],
        removedStackWidgets: ['binance']
      }
      expect(migrateStackWidgetSettings(initialState)).toEqual(expectedState)
    })

    it('migrates users who were showing binance and hiding rewards', () => {
      const initialState = {
        ...defaultState,
        currentStackWidget: 'binance',
        showBinance: true
      }
      const expectedState = {
        ...defaultState,
        showBinance: true,
        currentStackWidget: '',
        widgetStackOrder: ['binance'],
        removedStackWidgets: ['rewards']
      }
      expect(migrateStackWidgetSettings(initialState)).toEqual(expectedState)
    })

    it('preserves order and migrates users who were showing both widgets (Binance foreground)', () => {
      const initialState = {
        ...defaultState,
        currentStackWidget: 'binance',
        showBinance: true,
        showRewards: true
      }
      const expectedState = {
        ...defaultState,
        showBinance: true,
        showRewards: true,
        currentStackWidget: '',
        widgetStackOrder: ['rewards', 'binance'],
        removedStackWidgets: []
      }
      expect(migrateStackWidgetSettings(initialState)).toEqual(expectedState)
    })

    it('preserves order and migrates users who were showing both widgets (Rewards foreground)', () => {
      const initialState = {
        ...defaultState,
        currentStackWidget: 'rewards',
        showBinance: true,
        showRewards: true
      }
      const expectedState = {
        ...defaultState,
        showBinance: true,
        showRewards: true,
        currentStackWidget: '',
        widgetStackOrder: ['binance', 'rewards'],
        removedStackWidgets: []
      }
      expect(migrateStackWidgetSettings(initialState)).toEqual(expectedState)
    })
  })

  describe('replaceStackWidgets', () => {
    it('adds back widgets that should be showing in the stack', () => {
      const initialState = {
        ...defaultState,
        showBinance: true,
        showRewards: true,
        widgetStackOrder: ['binance']
      }
      const expectedState = {
        ...initialState,
        widgetStackOrder: ['rewards', 'binance']
      }
      expect(replaceStackWidgets(initialState)).toEqual(expectedState)
    })
  })
})
