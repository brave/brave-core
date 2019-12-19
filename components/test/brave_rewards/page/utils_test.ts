/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { convertBalance, formatConverted } from '../../../brave_rewards/resources/page/utils'

describe('Rewards Settings - Utils', () => {
  describe('convertBalance', () => {
    it('rates are empty', () => {
      expect(convertBalance(10.0, {})).toBe('0.00')
    })

    it('rate is missing', () => {
      expect(convertBalance(10.0, { 'USD': 10 }, 'EUR')).toBe('0.00')
    })

    it('all good', () => {
      expect(convertBalance(10.0, { 'USD': 10 })).toBe('100.00')
    })

    it('currency is provided', () => {
      expect(convertBalance(10.0, { 'USD': 10, 'EUR': 4 }, 'EUR')).toBe('40.00')
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
