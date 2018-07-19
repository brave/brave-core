/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import reducers from '../../../brave_welcome_ui/reducers/index'
import { welcomeInitialState } from '../../testData'

describe('welcome reducers test', () => {
  it('reducers are a combined reducer function', () => {
    expect(typeof reducers).toBe('function')
  })

  it('reducers passed with an unknown action returns the same input', () => {
    const unknownAction = {
      type: 'A LANNISTER ALWAYS PAYS HIS BATS'
    }

    const state = { ...welcomeInitialState }
    expect(reducers(state, unknownAction)).toBe(state)
  })
})
