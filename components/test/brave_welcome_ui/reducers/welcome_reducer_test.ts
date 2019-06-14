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
      searchProviders: []
    })
  })

  describe('IMPORT_NOW_REQUESTED', () => {
    let importNowRequestStub: jest.SpyInstance

    beforeEach(() => {
      importNowRequestStub = jest.spyOn(chrome, 'send')
    })

    afterEach(() => {
      importNowRequestStub.mockRestore()
    })

    it('should call chrome.send with the correct arguments', () => {
      welcomeReducer(undefined, {
        type: types.IMPORT_NOW_REQUESTED
      })

      expect(importNowRequestStub).toBeCalledTimes(1)
      expect(importNowRequestStub).toBeCalledWith('importNowRequested', [])
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

  describe('CLOSE_TAB_REQUESTED', () => {
    let closeTabRequestStub: jest.SpyInstance

    beforeEach(() => {
      closeTabRequestStub = jest.spyOn(window, 'close')
    })

    afterEach(() => {
      closeTabRequestStub.mockRestore()
    })

    it('calls window.close', () => {
      welcomeReducer(undefined, {
        type: types.CLOSE_TAB_REQUESTED,
        payload: undefined
      })
      expect(closeTabRequestStub).toBeCalledTimes(1)
    })
  })

  describe('CHANGE_DEFAULT_SEARCH_PROVIDER', () => {
    let changeSearchProviderStub: jest.SpyInstance

    beforeEach(() => {
      changeSearchProviderStub = jest.spyOn(chrome, 'send')
    })

    afterEach(() => {
      changeSearchProviderStub.mockRestore()
    })

    it('should call chrome.send with the correct argument', () => {
      welcomeReducer(undefined, {
        type: types.CHANGE_DEFAULT_SEARCH_PROVIDER,
        payload: '12345'
      })
      expect(changeSearchProviderStub).toBeCalledTimes(1)
      expect(changeSearchProviderStub).toBeCalledWith('setDefaultSearchEngine', [12345])
    })
  })
})
