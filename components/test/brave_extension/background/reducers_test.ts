/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import reducers from '../../../brave_extension/extension/brave_extension/background/reducers'
import { initialState } from '../../testData'

describe('reducers test', () => {
  it('reducers is a combined reducer function', () => {
    expect(typeof reducers).toBe('function')
  })

  it('reducers passed with an unknown action returns the same input', () => {
    const unknownAction = {
      type: 'DA07BE00-43FB-44C3-B020-49308A0D3E78'
    }
    const state = { ...initialState }
    expect(reducers(state, unknownAction)).toEqual(state)
  })
})
