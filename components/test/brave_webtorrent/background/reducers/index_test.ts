/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import reducers from '../../../../brave_webtorrent/extension/background/reducers/index'
import { applicationState } from '../../testData'

// this import seems to trigger createStore and get an undefined reducer
jest.mock('../../../../brave_webtorrent/extension/background/events/torrentEvents')

describe('webtorrent reducers test', () => {
  it('reducers are a combined reducer function', () => {
    expect(typeof reducers).toBe('function')
  })

  it('reducers passed with an unknown action returns the same input', () => {
    const unknownAction = { type: 'unknown' }
    const state = { ...applicationState }
    expect(reducers(state, unknownAction)).toBe(state)
  })
})
