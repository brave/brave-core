// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../constants/types'

import { mockAccount } from '../common/constants/mocks'

import { isHardwareAccount, findAccountFromRegistry } from './account-utils'

import {
  AccountInfoEntity,
  accountInfoEntityAdaptor
} from '../common/slices/entities/account-info.entity'

const mockHardware = {
  deviceId: 'testDeviceId',
  path: '',
  vendor: 'Ledger'
}

const mockHardwareAccounts: BraveWallet.AccountInfo[] = [
  {
    ...mockAccount,
    hardware: mockHardware
  },
  {
    ...mockAccount,
    address: 'mockAccount2',
    hardware: {
      ...mockHardware,
      deviceId: 'testDeviceId2'
    }
  }
]

const mockAccounts: AccountInfoEntity[] = [
  {
    ...mockAccount,
    deviceId: ''
  },
  {
    ...mockAccount,
    address: 'mockAccount2',
    deviceId: ''
  }
]

const mockAccountsRegistry = accountInfoEntityAdaptor.setAll(
  accountInfoEntityAdaptor.getInitialState(),
  mockAccounts
)

describe('Account Utils', () => {
  describe('isHardwareAccount', () => {
    it.each(mockHardwareAccounts)(
      'should return true if accounts have deviceId and address matches',
      (account) => {
        expect(isHardwareAccount(account)).toBe(true)
      }
    )
  })
  describe('findAccountFromRegistry', () => {
    it.each(mockAccounts)(
      'should return true if accounts have deviceId and address matches',
      (account) => {
        expect(
          findAccountFromRegistry(account.address, mockAccountsRegistry)
        ).toBe(account)
      }
    )
  })
})
