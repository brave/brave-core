/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import store from '../../../brave_extension/extension/brave_extension/background/store'
import { initialState } from '../../testData'

describe('store test', () => {
  it('store can get state', () => {
    expect(store.getState()).toEqual(initialState)
  })
})
