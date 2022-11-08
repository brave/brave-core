// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { mojoTimeDeltaToJSDate } from './mojomUtils'

describe('Convertting from MojoTime works', () => {
  test('0 epoch time value', () => {
    // Epoch time
    expect(mojoTimeDeltaToJSDate({ microseconds: 0 })).toEqual(new Date(null))
  })
  test('converts normal values ok', () => {
    const milliseconds = 1635363887716
    const microseconds = milliseconds * 1000
    const expectedDate = new Date()
    expectedDate.setTime(milliseconds)
    expect(mojoTimeDeltaToJSDate({ microseconds })).toEqual(expectedDate)
  })
})
