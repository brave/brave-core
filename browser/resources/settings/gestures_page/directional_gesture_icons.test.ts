/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {gestureKeyToIconNames} from './directional_gesture_icons'

describe('gestureKeyToIconNames', () => {
  test('should return an empty array for an empty key', () => {
    expect(gestureKeyToIconNames('')).toEqual([])
  })

  test('should map a single stroke to its icon name', () => {
    expect(gestureKeyToIconNames('↑')).toEqual(['arrow-up'])
  })

  test('should map each stroke in a multi-stroke key in order', () => {
    expect(gestureKeyToIconNames('↓→')).toEqual(['arrow-down', 'arrow-right'])
  })

  test('should map every diagonal stroke to its own icon name', () => {
    expect(gestureKeyToIconNames('↖↗↙↘')).toEqual([
      'arrow-diagonal-up-left', 'arrow-diagonal-up-right',
      'arrow-diagonal-down-left', 'arrow-diagonal-down-right',
    ])
  })

  test('should pass through a character with no known icon name', () => {
    expect(gestureKeyToIconNames('?')).toEqual(['?'])
  })
})
