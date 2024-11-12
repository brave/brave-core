/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Reducer
import { stackWidgetReducer, handleWidgetPrefsChange } from '../../../brave_new_tab_ui/reducers/stack_widget_reducer'

// API
import * as storage from '../../../brave_new_tab_ui/storage/new_tab_storage'
import { types } from '../../../brave_new_tab_ui/constants/stack_widget_types'

describe('stackWidgetReducer', () => {
  describe('SET_FOREGROUND_STACK_WIDGET', () => {
    it('adds widget if it is not in the stack and sets it to the foreground', () => {
      const assertion = stackWidgetReducer({
        ...storage.defaultState,
        widgetStackOrder: ['rewards']
      }, {
        type: types.SET_FOREGROUND_STACK_WIDGET,
        payload: {
          widget: 'braveTalk'
        }
      })
      const expectedState = {
        ...storage.defaultState,
        widgetStackOrder: ['rewards', 'braveTalk']
      }
      expect(assertion).toEqual(expectedState)
    })

    it('sets a widget to the foreground if it is in the stack', () => {
      const assertion = stackWidgetReducer({
        ...storage.defaultState,
        widgetStackOrder: ['braveTalk', 'rewards']
      }, {
        type: types.SET_FOREGROUND_STACK_WIDGET,
        payload: {
          widget: 'braveTalk'
        }
      })
      const expectedState = {
        ...storage.defaultState,
        widgetStackOrder: ['rewards', 'braveTalk']
      }
      expect(assertion).toEqual(expectedState)
    })

    it('does not re-add a widget', () => {
      const assertion = stackWidgetReducer({
        ...storage.defaultState,
        widgetStackOrder: ['braveTalk', 'rewards']
      }, {
        type: types.SET_FOREGROUND_STACK_WIDGET,
        payload: {
          widget: 'rewards'
        }
      })
      const expectedState = {
        ...storage.defaultState,
        widgetStackOrder: ['braveTalk', 'rewards']
      }
      expect(assertion).toEqual(expectedState)
    })
  })

  describe('addNewStackWidget', () => {
    it('puts new widget at the top at first', () => {
      const state = {
        ...storage.defaultState,
        widgetStackOrder: ['braveTalk', 'rewards']
      }

      // Last item in |widgetStackOrder| is top-most card.
      const expectedState = {
        ...storage.defaultState,
        widgetStackOrder: ['braveTalk', 'rewards', 'braveVPN']
      }

      const assertion = storage.addNewStackWidget(state)
      expect(assertion).toEqual(expectedState)
    })
  })

  describe('handleWidgetPrefsChange', () => {
    it('puts a widget in the forgeround if it is being turned back on', () => {
      const oldState = {
        ...storage.defaultState,
        showBraveTalk: false
      }
      const newState = {
        ...storage.defaultState,
        showBraveTalk: true
      }
      const expectedState = {
        ...storage.defaultState,
        showBraveTalk: true,
        widgetStackOrder: ['rewards', 'braveVPN', 'braveTalk']
      }
      const assertion = handleWidgetPrefsChange(newState, oldState)
      expect(assertion).toEqual(expectedState)

      // Test again for vpn widget.
      newState.showBraveTalk = false
      expectedState.showBraveTalk = false;
      newState.showBraveVPN = true
      expectedState.showBraveVPN = true
      expectedState.widgetStackOrder = ['rewards', 'braveVPN']

      const assertionForVPN = handleWidgetPrefsChange(newState, oldState)
      expect(assertionForVPN).toEqual(expectedState)
    })
    it('removes a widget from the stack if its being turned off', () => {
      const oldState = {
        ...storage.defaultState,
        showBraveTalk: true
      }
      const newState = {
        ...storage.defaultState,
        showBraveTalk: false
      }
      const expectedState = {
        ...storage.defaultState,
        showBraveTalk: false,
        removedStackWidgets: ['braveTalk']
      }
      const assertion = handleWidgetPrefsChange(newState, oldState)
      expect(assertion).toEqual(expectedState)
    })
  })
})
