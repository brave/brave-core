// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { isHardwareAccount, isValidAddress, isValidFilAddress } from './address-utils'
import { mockAddresses, mockAccount, mockFilAddresses, mockFilInvalilAddresses } from '../common/constants/mocks'
import { WalletAccountType } from '../constants/types'

const validAdresses = mockAddresses.map((addr: string) => [addr, true])
const invalidAddresses = [
  '0xea674fdde714fd979de3edf0f56aa9716b898',
  '0xdbf41e98f541f19bb044e604d2520f3893e',
  '0xcee177039c99d03a6f74e95bb23ceea43ea2'
].map(addr => [addr, false])

const validFilAdresses = mockFilAddresses.map((addr: string) => [addr, true])
const validFilInvalidAdresses = mockFilInvalilAddresses.map((addr: string) => [addr, false])

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

  describe('isHardwareAddress', () => {
    it('should return true if accounts have deviceId and address matches', () => {
      const accounts: WalletAccountType[] = [
        {
          ...mockAccount,
          deviceId: 'testDeviceId'
        },
        {
          ...mockAccount,
          address: 'mockAccount2',
          deviceId: 'testDeviceId2'
        }
      ]
      expect(isHardwareAccount(accounts, mockAccount.address)).toBe(true)
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
})
