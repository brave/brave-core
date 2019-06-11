/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import welcomeReducer from '../../../brave_welcome_ui/reducers/welcome_reducer'
import * as actions from '../../../brave_welcome_ui/actions/welcome_actions'
import { types } from '../../../brave_welcome_ui/constants/welcome_types'

window.open = jest.fn()
window.close = jest.fn()

describe('welcomeReducer', () => {
  it('should handle initial state', () => {
    const assertion = welcomeReducer(undefined, actions.closeTabRequested())
    expect(assertion).toEqual({
      defaultSearchProviders: []
    })
  })

  describe.skip('IMPORT_NOW_REQUESTED', () => {
    it('calls importNowRequested', () => {
      // TODO
    })
  })

  describe('GO_TO_TAB_REQUESTED', () => {
    it('calls window.open', () => {
      welcomeReducer(undefined, {
        type: types.GO_TO_TAB_REQUESTED,
        payload: { url: 'https://brave.com', target: '_blank' }
      })
      expect(window.open).toBeCalled()
    })
  })

  describe.skip('CLOSE_TAB_REQUESTED', () => {
    it('calls window.close', () => {
      welcomeReducer(undefined, {
        type: types.CLOSE_TAB_REQUESTED,
        payload: undefined
      })
      expect(window.close).toBeCalled()
    })
  })

  describe('IMPORT_DEFAULT_SEARCH_PROVIDERS', () => {
    it('should update state with new search providers', () => {
      const newState = welcomeReducer(undefined, actions.importDefaultSearchProviders(['DuckDuckGo', 'Google']))
      const expected = {
        defaultSearchProviders: ['DuckDuckGo', 'Google']
      }
      expect(newState).toEqual(expected)
    })
  })
})
