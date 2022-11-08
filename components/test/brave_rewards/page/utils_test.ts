/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
/* global describe, it */

import { convertBalance } from '../../../brave_rewards/resources/page/components/utils'

describe('Rewards Settings - Utils', () => {
  describe('convertBalance', () => {
    it('rate is 0', () => {
      expect(convertBalance(10.0, 0)).toBe('0.00')
    })

    it('all good', () => {
      expect(convertBalance(10.0, 10)).toBe('100.00')
    })
  })
})
