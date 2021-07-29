/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { getCurrentMonthRange } from './wallet_card'

function mockNow (timeString: string) {
  jest.spyOn(Date, 'now').mockImplementation(() => Date.parse(timeString))
}

describe('wallet_card', () => {

  describe('getCurrentMonthRange', () => {
    it('returns a date range for the current month', () => {
      mockNow('2021-01-01T00:00:00')
      expect(getCurrentMonthRange()).toStrictEqual('Jan 1 – Jan 31')

      mockNow('2021-01-20T00:00:00')
      expect(getCurrentMonthRange()).toStrictEqual('Jan 1 – Jan 31')

      mockNow('2021-01-31T23:00:00')
      expect(getCurrentMonthRange()).toStrictEqual('Jan 1 – Jan 31')
    })
  })

})
