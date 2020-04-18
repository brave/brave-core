// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as binanceUtils from '../../../brave_new_tab_ui/binance-utils'

describe('binance utilities tests', () => {
  describe('isValidClientURL', () => {
    it('handles falsey urls', () => {
      expect(binanceUtils.isValidClientURL('')).toBe(false)
      expect(binanceUtils.isValidClientURL(null)).toBe(false)
    })

    it('handles non-url strings', () => {
      expect(binanceUtils.isValidClientURL('httpsurl')).toBe(false)
      expect(binanceUtils.isValidClientURL('abcdefgh')).toBe(false)
    })

    it('handles non https protocols', () => {
      expect(binanceUtils.isValidClientURL('http://accounts.binance.com/en/oauth/authorize')).toBe(false)
    })

    it('handles unexpected hosts', () => {
      expect(binanceUtils.isValidClientURL('https://accounts.fake-binance.com/en/oauth/authorize')).toBe(false)
    })

    it('handles unexpected paths', () => {
      expect(binanceUtils.isValidClientURL('https://accounts.binance.com/en/oauth/not-authorize')).toBe(false)
    })

    it('accepts an expected url', () => {
      expect(binanceUtils.isValidClientURL('https://accounts.binance.com/en/oauth/authorize?response_type=code&client_id=some_id&redirect_uri=com.brave.binance%3A%2F%2Fauthorization&scope=user%3Aemail%2Cuser%3Aaddress%2Casset%3Abalance%2Casset%3Aocbs&code_challenge=code&code_challenge_method=S256')).toBe(true)
    })
  })
})
