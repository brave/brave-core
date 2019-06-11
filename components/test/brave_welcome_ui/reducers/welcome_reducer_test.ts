/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as sinon from 'sinon'
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
    let importNowRequestStub

    beforeEach(() => {
      importNowRequestStub = sinon.stub(chrome, 'send').resolves()
    })

    afterEach(() => {
      importNowRequestStub.restore()
    })

    it('should call chrome.send with the correct arguments', () => {
      welcomeReducer(undefined, {
        type: types.IMPORT_NOW_REQUESTED
      })

      sinon.assert.calledOnce(importNowRequestStub)
      sinon.assert.calledWith(importNowRequestStub, 'importNowRequested')
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
    let closeTabRequestStub

    beforeEach(() => {
      closeTabRequestStub = sinon.stub(window, 'close').resolves()
    })

    afterEach(() => {
      closeTabRequestStub.restore()
    })

    it('calls window.close', () => {
      welcomeReducer(undefined, {
        type: types.CLOSE_TAB_REQUESTED,
        payload: undefined
      })
      sinon.assert.calledOnce(closeTabRequestStub)
    })
  })

  describe('CHANGE_DEFAULT_SEARCH_PROVIDER', () => {
    let changeSearchProviderStub

    beforeEach(() => {
      changeSearchProviderStub = sinon.stub(chrome, 'send').resolves()
    })

    afterEach(() => {
      changeSearchProviderStub.restore()
    })

    it('should call chrome.send with the correct argument', () => {
      welcomeReducer(undefined, {
        type: types.CHANGE_DEFAULT_SEARCH_PROVIDER,
        payload: '12345'
      })
      sinon.assert.calledOnce(changeSearchProviderStub)
      sinon.assert.calledWith(changeSearchProviderStub, 'setDefaultSearchEngine', [12345])
    })
  })
})
