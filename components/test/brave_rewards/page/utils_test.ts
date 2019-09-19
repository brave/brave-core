/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { convertBalance, formatConverted, convertProbiToFixed } from '../../../brave_rewards/resources/page/utils'

describe('Rewards Settings - Utils', () => {
  describe('convertBalance', () => {
    it('token has letters', () => {
      expect(convertBalance('test', { 'USD': 10 })).toBe('0.00')
    })

    it('rates are empty', () => {
      expect(convertBalance('10', {})).toBe('0.00')
    })

    it('rate is missing', () => {
      expect(convertBalance('10', { 'USD': 10 }, 'EUR')).toBe('0.00')
    })

    it('all good', () => {
      expect(convertBalance('10', { 'USD': 10 })).toBe('100.00')
    })

    it('currency is provided', () => {
      expect(convertBalance('10', { 'USD': 10, 'EUR': 4 }, 'EUR')).toBe('40.00')
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

  describe('convertProbiToFixed', () => {
    it('probi is not in correct format', () => {
      expect(convertProbiToFixed('sdfs')).toBe('0.0')
    })

    it('probi is not 10^18', () => {
      expect(convertProbiToFixed('9')).toBe('0.0')
    })

    it('we should always round down', () => {
      expect(convertProbiToFixed('0999999999999999999')).toBe('0.9')
    })

    it('regular convert', () => {
      expect(convertProbiToFixed('1559999999999999990')).toBe('1.5')
    })

    it('regular convert two places', () => {
      expect(convertProbiToFixed('1559999999999999990', 2)).toBe('1.55')
    })

    it('big convert', () => {
      expect(convertProbiToFixed('150000000000000000000000000')).toBe('150000000.0')
    })
  })
})
