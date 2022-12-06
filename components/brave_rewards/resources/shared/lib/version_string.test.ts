/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { compareVersionStrings } from './version_string'

describe('version_string', () => {
  describe('compareVersionStrings', () => {
    it('throws for invalid strings', () => {
      expect(() => { compareVersionStrings('', '1') }).toThrow()
      expect(() => { compareVersionStrings('1', 'abc') }).toThrow()
      expect(() => { compareVersionStrings('abc', '1') }).toThrow()
      expect(() => { compareVersionStrings('1.2.*', '1') }).toThrow()
      expect(() => { compareVersionStrings('1.2.-12', '1') }).toThrow()
      expect(() => { compareVersionStrings('1.2abc', '1') }).toThrow()
      expect(() => { compareVersionStrings('1.2.0.', '1') }).toThrow()
    })

    it('compares version strings', () => {
      expect(compareVersionStrings('1.1.1', '1.1.1')).toEqual(0)
      expect(compareVersionStrings('1.1.1', '1.1.1.0.0')).toEqual(0)
      expect(compareVersionStrings('1.1.1.0.0', '1.1.1')).toEqual(0)
      expect(compareVersionStrings('1.1.1', '1.1.0')).toBeGreaterThan(0)
      expect(compareVersionStrings('1.1.0', '1.1.1')).toBeLessThan(0)
    })
  })
})
