/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { types } from '../../../brave_welcome_ui/constants/welcome_types'
import * as actions from '../../../brave_welcome_ui/actions/welcome_actions'
import { mockImportSources } from '../../testData'

describe('welcome_actions', () => {
  it('getBrowserProfilesSuccess', () => {
    expect(actions.getBrowserProfilesSuccess(mockImportSources)).toEqual({
      type: types.IMPORT_BROWSER_PROFILES_SUCCESS,
      meta: undefined,
      payload: mockImportSources
    })
  })

  it('importBrowserProfileRequested', () => {
    expect(actions.importBrowserProfileRequested(0)).toEqual({
      type: types.IMPORT_BROWSER_DATA_REQUESTED,
      meta: undefined,
      payload: 0
    })
  })

  it('goToTabRequested', () => {
    expect(actions.goToTabRequested('https://rossmoody.design', '_blank')).toEqual({
      type: types.GO_TO_TAB_REQUESTED,
      meta: undefined,
      payload: { url: 'https://rossmoody.design', target: '_blank' }
    })
  })

  it('closeTabRequested', () => {
    expect(actions.closeTabRequested()).toEqual({
      type: types.CLOSE_TAB_REQUESTED,
      meta: undefined,
      payload: undefined
    })
  })

  it('changeDefaultSearchProvider', () => {
    expect(actions.changeDefaultSearchProvider(12345)).toEqual({
      type: types.CHANGE_DEFAULT_SEARCH_PROVIDER,
      payload: 12345
    })
  })

  it('getSearchEngineProvidersSuccess', () => {
    const mockPayload = []
    expect(actions.getSearchEngineProvidersSuccess(mockPayload)).toEqual({
      type: types.IMPORT_DEFAULT_SEARCH_PROVIDERS_SUCCESS,
      payload: mockPayload
    })
  })
})
