// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import {
  isValidAddress,
  isValidEVMAddress,
  isValidSolanaAddress,
  isValidFilAddress,
  isValidBtcAddress
} from './address-utils'
import {
  mockAddresses,
  mockFilAddresses,
  mockFilInvalilAddresses,
  mockSolanaAccount
} from '../common/constants/mocks'

const validAdresses = mockAddresses.map((addr: string) => [addr, true])
const invalidAddresses = [
  '0xea674fdde714fd979de3edf0f56aa9716b898',
  '0xdbf41e98f541f19bb044e604d2520f3893e',
  '0xcee177039c99d03a6f74e95bb23ceea43ea2'
].map((addr) => [addr, false])

const validFilAdresses = mockFilAddresses.map((addr: string) => [addr, true])
const validFilInvalidAdresses = mockFilInvalilAddresses.map((addr: string) => [
  addr,
  false
])
const validSolanaAddress = mockSolanaAccount.address

const validBtcMainnetAddresses = [
  '1FsSia9rv4NeEwvJ2GvXrX7LyxYspbN2mo',
  '36j4NfKv6Akva9amjWrLG6MuSQym1GuEmm',
  'bc1qvyq0cc6rahyvsazfdje0twl7ez82ndmuac2lhv',
  'bc1qyucykdlhp62tezs0hagqury402qwhk589q80tqs5myh3rxq34nwqhkdhv7',
  'bc1p83n3au0rjylefxq2nc2xh2y4jzz4pm6zxj4mw5pagdjjr2a9f36s6jjnnu',
  '1FjL87pn8ky6Vbavd1ZHeChRXtoxwRGCRd',
  '3BZECeAH8gSKkjrTx8PwMrNQBLG18yHpvf',
  'bc1qhxt04s5xnpy0kxw4x99n5hpdf5pmtzpqs52es2',
  'bc1qgc9ljrvdf2e0zg9rmmq86xklqwfys7r6wptjlacdgrcdc7sa6ggqu4rrxf',
  'bc1pve739yap4uxjvfk0jrey69078u0gasm2nwvv483ec6zkzulgw9xqu4w9fd',
  '1G9A9j6W8TLuh6dEeVwWeyibK1Uc5MfVFV',
  '33GA3ZXbw5o5HeUrBEaqkWXFYYZmdxGRRP'
]

const validBtcTestnetAddresses = [
  'mzK2FFDEhxqHcmrJw1ysqFkVyhUULo45hZ',
  '2NC2hEhe28ULKAJkW5MjZ3jtTMJdvXmByvK',
  'tb1qcrh3yqn4nlleplcez2yndq2ry8h9ncg3qh7n54',
  'tb1quyl9ujpgwr2chdzdnnalen48sup245vdfnh2jxhsuq3yx80rrwlq5hqfe4',
  'tb1p35n52jy6xkm4wd905tdy8qtagrn73kqdz73xe4zxpvq9t3fp50aqk3s6gz',
  'n4YNbYuFdPwFrxSP8sjHFbAhUbLMUiY9jE',
  '2NAeQVZayzVFAtgeC3iYJsjpjWDmsDph71A',
  'tb1ql4k5ayv7p7w0t0ge7tpntgpkgw53g2payxkszr',
  'tb1q9jx3x2qqdpempxrcfgyrkjd5fzeacaqj4ua7cs7fe2sfd2wdaueq5wn26y',
  'tb1pdswckwd9ym5yf5eyzg8j4jjwnzla8y0tf9cp7aasfkek0u29sz9qfr00yf',
  'mwgS2HRbjyfYxFnR1nF9VKLvmdgMfFBmGq',
  '2MwBVrJQ76BdaGD76CTmou8cZzQYLpe4NqU'
]

describe('Address Utils', () => {
  describe('isValidAddress', () => {
    it.each(validAdresses)(
      'should return true if address is valid',
      (address: string, isValid: boolean) => {
        expect(isValid).toBe(isValidAddress(address, 20))
      }
    )

    it.each(invalidAddresses)(
      'should return false if address is invalid',
      (address: string, isValid: boolean) => {
        expect(isValid).toBe(isValidAddress(address, 20))
      }
    )

    it('should return false if address length is not (2 + 2) * length', () => {
      const testLength = 10
      const testAddress = mockAddresses[0]
      expect(isValidAddress(testAddress, testLength)).toBe(false)
    })

    it('should return false if address does not start with 0x', () => {
      const testAddress = mockAddresses[1].substring(3) // exclude 0x
      expect(isValidAddress(testAddress, 20)).toBe(false)
    })
  })

  describe('isValidFilAddress', () => {
    it.each(validFilAdresses)(
      'should return true if address is valid',
      (address: string, isValid: boolean) => {
        expect(isValid).toBe(isValidFilAddress(address))
      }
    )

    it.each(validFilInvalidAdresses)(
      'should return false if address is invalid',
      (address: string, isValid: boolean) => {
        expect(isValid).toBe(isValidFilAddress(address))
      }
    )
  })

  describe('isValidEVMAddress', () => {
    it.each(validAdresses)(
      'should return true if address is valid',
      (address: string, isValid: boolean) => {
        expect(isValid).toBe(isValidEVMAddress(address))
      }
    )

    it.each(invalidAddresses)(
      'should return false if address is invalid',
      (address: string, isValid: boolean) => {
        expect(isValid).toBe(isValidEVMAddress(address))
      }
    )

    it('should return false if address length is invalid', () => {
      expect(isValidEVMAddress('0xdeadbeef')).toBe(false)
      expect(isValidEVMAddress(mockAddresses[1] + '0')).toBe(false)
    })

    it('should return false if address does not start with 0x', () => {
      const testAddress = mockAddresses[1].substring(3) // exclude 0x
      expect(isValidEVMAddress(testAddress)).toBe(false)
    })

    it('should return false if address contains invalid characters', () => {
      expect(
        isValidEVMAddress('0xdeadbeefdeadbeefdeadbeefdeadbeefdeadzzzz')
      ).toBe(false)
    })
  })

  describe('isValidSolanaAddress', () => {
    it('should return true if address is valid', () => {
      expect(isValidSolanaAddress(validSolanaAddress)).toBe(true)
    })

    it('should return false if address length is invalid', () => {
      expect(isValidSolanaAddress(validSolanaAddress.substring(0, 31))).toBe(
        false
      )
      expect(isValidSolanaAddress(validSolanaAddress + '4')).toBe(false)
    })

    it('should return false if address contains invalid characters', () => {
      expect(
        isValidSolanaAddress(validSolanaAddress.substring(0, 31) + '0')
      ).toBe(false)
    })
  })

  describe('isValidBtcAddress', () => {
    it.each(validBtcMainnetAddresses)(
      'should return true if address is valid',
      (address) => {
        expect(isValidBtcAddress(address, false)).toBe(true)
        expect(isValidBtcAddress(address, true)).toBe(false)
      }
    )
    it.each(validBtcTestnetAddresses)(
      'should return true if address is valid',
      (address) => {
        expect(isValidBtcAddress(address, true)).toBe(true)
        expect(isValidBtcAddress(address, false)).toBe(false)
      }
    )
  })
})
