/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import welcomeReducer from '../../../brave_welcome_ui/reducers/welcome_reducer'
import * as actions from '../../../brave_welcome_ui/actions/welcome_actions'
import * as storage from '../../../brave_welcome_ui/storage'
import { loadTimeData } from '../../../common/loadTimeData'
import { types } from '../../../brave_welcome_ui/constants/welcome_types'
import { mockSearchProviders, mockImportSources } from '../../testData'

window.open = jest.fn()
window.close = jest.fn()

describe('welcomeReducer', () => {
  describe('Handle initial state', () => {
    let spy: jest.SpyInstance
    beforeEach(() => {
      spy = jest.spyOn(storage, 'load')
    })
    afterEach(() => {
      spy.mockRestore()
    })
    it('calls storage.load() when initial state is undefined', () => {
      const assertion = welcomeReducer(undefined, actions.closeTabRequested())
      expect(assertion).toEqual({
        searchProviders: [],
        browserProfiles: []
      })
      expect(spy).toBeCalled()
      expect(spy.mock.calls[0][1]).toBe(undefined)
    })
  })

  describe('IMPORT_BROWSER_DATA_REQUESTED', () => {
    let importBrowserProfileRequestStub: jest.SpyInstance

    beforeEach(() => {
      importBrowserProfileRequestStub = jest.spyOn(chrome, 'send')
    })

    afterEach(() => {
      importBrowserProfileRequestStub.mockRestore()
    })

    it('should call chrome.send with the correct arguments', () => {
      welcomeReducer(undefined, {
        type: types.IMPORT_BROWSER_DATA_REQUESTED,
        payload: 0
      })

      const importTypes = {
        import_dialog_autofill_form_data: true,
        import_dialog_bookmarks: true,
        import_dialog_history: true,
        import_dialog_saved_passwords: true,
        import_dialog_search_engine: true,
        import_dialog_extensions: true,
        import_dialog_payments: true
      }

      expect(importBrowserProfileRequestStub).toBeCalledTimes(1)
      expect(importBrowserProfileRequestStub).toBeCalledWith('importData', [0, importTypes])
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

  describe('IMPORT_DEFAULT_SEARCH_PROVIDERS_SUCCESS', () => {
    it('should set default search provider data', () => {
      const mockState = {
        searchProviders: [],
        browserProfiles: []
      }
      const result = welcomeReducer(mockState, {
        type: types.IMPORT_DEFAULT_SEARCH_PROVIDERS_SUCCESS,
        payload: mockSearchProviders
      })
      const expected = {
        ...mockState,
        searchProviders: mockSearchProviders
      }
      expect(result).toEqual(expected)
    })

    describe('with the region', () => {
      const mockState = {
        searchProviders: [],
        browserProfiles: []
      }
      const examplePayload = [
        { name: 'Google', canBeRemoved: false },
        { name: 'Brave Search beta', canBeRemoved: true }
      ]
      let countryString: string = 'US'
      loadTimeData = {
        getString: (fieldName: string) => {
          switch (fieldName) {
            case 'countryString': return countryString
          }
          return undefined
        }
      }
      let spy: jest.SpyInstance

      beforeEach(() => {
        spy = jest.spyOn(loadTimeData, 'getString')
      })
      afterEach(() => {
        spy.mockRestore()
      })

      it('should get the country string', () => {
        welcomeReducer(mockState, {
          type: types.IMPORT_DEFAULT_SEARCH_PROVIDERS_SUCCESS,
          payload: examplePayload
        })
        expect(spy).toBeCalledWith('countryString')
      })

      describe('when user is in US/Canada', () => {
        it('should NOT filter out the Brave engine', () => {
          const result = welcomeReducer(mockState, {
            type: types.IMPORT_DEFAULT_SEARCH_PROVIDERS_SUCCESS,
            payload: examplePayload
          })
          expect(result.searchProviders.length).toEqual(2)
        })
      })

      describe('when user is NOT in US/Canada', () => {
        beforeEach(() => {
          countryString = 'GB'
        })
        afterEach(() => {
          countryString = 'US'
        })
        it('should filter out Brave', () => {
          const result = welcomeReducer(mockState, {
            type: types.IMPORT_DEFAULT_SEARCH_PROVIDERS_SUCCESS,
            payload: examplePayload
          })
          expect(result.searchProviders.length).toEqual(1)
        })

        it('should leave Brave if its set as default', () => {
          const result = welcomeReducer(mockState, {
            type: types.IMPORT_DEFAULT_SEARCH_PROVIDERS_SUCCESS,
            payload: [
              { name: 'Google', canBeRemoved: true },
              { name: 'Brave Search beta', canBeRemoved: false }
            ]
          })
          expect(result.searchProviders.length).toEqual(2)
        })
      })
    })
  })

  describe('IMPORT_BROWSER_PROFILES_SUCCESS', () => {
    it('should set import browser profile data', () => {
      const mockState = {
        searchProviders: [],
        browserProfiles: []
      }
      const result = welcomeReducer(mockState, {
        type: types.IMPORT_BROWSER_PROFILES_SUCCESS,
        payload: mockImportSources
      })
      const expected = {
        ...mockState,
        browserProfiles: mockImportSources
      }
      expect(result).toEqual(expected)
    })
  })
})
