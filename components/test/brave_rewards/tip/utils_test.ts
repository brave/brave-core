/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { convertBalance } from '../../../brave_rewards/resources/tip/utils'

describe('Rewards Panel extension - Utils', () => {
  describe('convertBalance', () => {
    it('token has letters', () => {
      expect(convertBalance('test', 10)).toBe('0.00')
    })

    it('rate is 0', () => {
      expect(convertBalance('10', 0)).toBe('0.00')
    })

    it('all good', () => {
      expect(convertBalance('10', 10)).toBe('100.00')
    })
  })
})
