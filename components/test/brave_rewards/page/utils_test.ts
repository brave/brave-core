/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { convertBalance, formatConverted } from '../../../brave_rewards/resources/page/utils'

describe('Rewards Settings - Utils', () => {
  describe('convertBalance', () => {
    it('token has letters', () => {
      expect(convertBalance('test', 10)).toBe('0.00')
    })

    it('rate is 0', () => {
      expect(convertBalance(10.0, 0)).toBe('0.00')
    })

    it('all good', () => {
      expect(convertBalance(10.0, 10)).toBe('100.00')
    })
  })

  describe('formatConverted', () => {
    it('currency is not provided', () => {
      expect(formatConverted('10.00')).toBe('10.00 USD')
    })

    it('currency is provided', () => {
      expect(formatConverted('10.00', 'EUR')).toBe('10.00 EUR')
    })
  })
})
