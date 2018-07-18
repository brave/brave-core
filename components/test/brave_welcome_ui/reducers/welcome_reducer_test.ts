/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import welcomeReducer from '../../../brave_welcome_ui/reducers/welcome_reducer'
import * as actions from '../../../brave_welcome_ui/actions/welcome_actions'
import { types } from '../../../brave_welcome_ui/constants/welcome_types'
import { initialState } from '../../testData'

describe('welcomeReducer', () => {
  it('should handle initial state', () => {
    const assertion = welcomeReducer(undefined, actions.goToPageRequested(1))
    expect(assertion).toEqual({ pageIndex: 1 })
  })

  describe('GO_TO_PAGE_REQUESTED', () => {
    it('sets the pageIndex', () => {
      const assertion = welcomeReducer(initialState.welcomeData, {
        type: types.GO_TO_PAGE_REQUESTED,
        payload: { pageIndex: 1337 }
      })
      expect(assertion).toEqual({ pageIndex: 1337 })
     })
   })

   describe.skip('IMPORT_NOW_REQUESTED', () => {
    it('calls importNowRequested', () => {
      // TODO
    })
  })
 })