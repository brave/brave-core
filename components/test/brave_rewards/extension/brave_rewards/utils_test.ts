/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { convertBalance, handleContributionAmount } from '../../../../brave_rewards/resources/extension/brave_rewards/utils'

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

  describe('handleContributionAmount', () => {
    it('amount is 0', () => {
      expect(handleContributionAmount('0')).toBe('0.000')
    })
    it('amount is in wrong format', () => {
      expect(handleContributionAmount('dasdfasdfasdf')).toBe('0.000')
    })
    it('amount is probi', () => {
      expect(handleContributionAmount('1000000000000000000')).toBe('1.000')
    })
    it('amount is probi', () => {
      expect(handleContributionAmount('10454500000000000000')).toBe('10.455')
    })
    it('amount is double', () => {
      expect(handleContributionAmount('10.454545')).toBe('10.455')
    })
    it('amount is int', () => {
      expect(handleContributionAmount('10')).toBe('10.000')
    })
  })
})
