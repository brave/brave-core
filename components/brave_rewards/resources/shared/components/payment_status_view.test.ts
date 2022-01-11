/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import {
  getDaysUntilRewardsPayment,
  shouldShowRewardsPaymentCompleted
} from './payment_status_view'

function ms (timeString: string) {
  return new Date(timeString).getTime()
}

function mockNow (timeString: string) {
  jest.spyOn(Date, 'now').mockImplementation(() => ms(timeString))
}

describe('pending_rewards', () => {
  describe('shouldShowRewardsPaymentCompleted', () => {
    it('returns false if the next payment date is in the past', () => {
      mockNow('2021-01-20')
      expect(shouldShowRewardsPaymentCompleted(ms('2021-01-19')))
        .toStrictEqual(false)
    })

    it('returns true if we are 2 days past the previous payment date', () => {
      mockNow('2021-01-09')
      expect(shouldShowRewardsPaymentCompleted(ms('2021-02-07')))
        .toStrictEqual(true)
      expect(shouldShowRewardsPaymentCompleted(ms('2021-03-07')))
        .toStrictEqual(true)
    })

    it('returns false if we are 4 days past the previous payment date', () => {
      mockNow('2021-01-11')
      expect(shouldShowRewardsPaymentCompleted(ms('2021-02-07')))
        .toStrictEqual(false)
    })
  })

  describe('getDaysUntilRewardsPayment', () => {
    it('returns empty if month does not match', () => {
      mockNow('2021-01-20')
      expect(getDaysUntilRewardsPayment(ms('2021-02-05'))).toStrictEqual('')
    })

    it('rounds payment date down to midnight local time', () => {
      mockNow('2021-01-01T00:00:01')
      expect(getDaysUntilRewardsPayment(ms('2021-01-04T12:00:00')))
        .toStrictEqual('3 days')
    })

    it('returns empty if payment date is in the past', () => {
      mockNow('2021-01-06T09:00:00')
      expect(getDaysUntilRewardsPayment(ms('2021-01-05T14:00:00')))
        .toStrictEqual('')
    })

    it('returns empty if payment date is later today', () => {
      mockNow('2021-01-05T09:00:00')
      expect(getDaysUntilRewardsPayment(ms('2021-01-05T14:00:00')))
        .toStrictEqual('')
    })

    it('rounds days up', () => {
      mockNow('2021-01-04T22:00:00')
      expect(getDaysUntilRewardsPayment(ms('2021-01-05T04:00:00')))
        .toStrictEqual('1 day')
    })
  })
})
