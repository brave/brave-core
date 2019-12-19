/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { convertBalance } from '../../../brave_rewards/resources/tip/utils'

describe('Rewards Panel extension - Utils', () => {
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
})
