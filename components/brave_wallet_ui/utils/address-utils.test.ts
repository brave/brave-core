// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import {
  isValidAddress,
  isValidEVMAddress,
  isValidSolanaAddress,
  isValidFilAddress
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
].map(addr => [addr, false])

const validFilAdresses = mockFilAddresses.map((addr: string) => [addr, true])
const validFilInvalidAdresses = mockFilInvalilAddresses.map((addr: string) => [addr, false])
const validSolanaAddress = mockSolanaAccount.address

describe('Address Utils', () => {
  describe('isValidAddress', () => {
    it.each(validAdresses)('should return true if address is valid', (address: string, isValid: boolean) => {
      expect(isValid).toBe(isValidAddress(address, 20))
    })

    it.each(invalidAddresses)('should return false if address is invalid', (address: string, isValid: boolean) => {
      expect(isValid).toBe(isValidAddress(address, 20))
    })

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
    it.each(validFilAdresses)('should return true if address is valid', (address: string, isValid: boolean) => {
      expect(isValid).toBe(isValidFilAddress(address))
    })

    it.each(validFilInvalidAdresses)('should return false if address is invalid', (address: string, isValid: boolean) => {
      expect(isValid).toBe(isValidFilAddress(address))
    })
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
      expect(isValidEVMAddress(mockAddresses[1] + "0")).toBe(false)
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
      expect(
        isValidSolanaAddress(validSolanaAddress.substring(0, 31))
      ).toBe(false)
      expect(isValidSolanaAddress(validSolanaAddress + "4")).toBe(false)
    })

    it('should return false if address contains invalid characters', () => {
      expect(
        isValidSolanaAddress(validSolanaAddress.substring(0, 31) + '0')
      ).toBe(false)
    })
  })
})
