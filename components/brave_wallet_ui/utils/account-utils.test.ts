// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../constants/types'

import {
  mockAccount,
  mockBitcoinAccount,
  mockBitcoinTestnetAccount,
  mockEthAccountInfo,
  mockFilecoinAccount,
  mockSolanaAccount
} from '../common/constants/mocks'

import {
  isHardwareAccount,
  findAccountByAddress,
  getAccountTypeDescription,
  findAccountByAccountId
} from './account-utils'

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
    accountId: {
      ...mockAccount.accountId,
      kind: BraveWallet.AccountKind.kHardware
    },
    hardware: mockHardware
  },
  {
    ...mockAccount,
    accountId: {
      ...mockAccount.accountId,
      kind: BraveWallet.AccountKind.kHardware,
      address: 'mockAccount2'
    },
    address: 'mockAccount2',
    hardware: {
      ...mockHardware,
      deviceId: 'testDeviceId2'
    }
  }
]

const mockAccounts: AccountInfoEntity[] = [
  mockAccount,
  mockEthAccountInfo,
  mockSolanaAccount,
  mockFilecoinAccount,
  mockBitcoinAccount,
  mockBitcoinTestnetAccount
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
        expect(isHardwareAccount(account.accountId)).toBe(true)
      }
    )
  })
  describe('findAccountFromRegistry', () => {
    it.each(mockAccounts)(
      'should return true if accounts have deviceId and address matches',
      (account) => {
        if (account.address) {
          expect(
            findAccountByAddress(account.address, mockAccountsRegistry)
          ).toBe(account)
        }
      }
    )
  })
  describe('findAccountByAccountId', () => {
    it.each(mockAccounts)(
      'should return true if accounts have accountId matches',
      (account) => {
        expect(
          findAccountByAccountId(account.accountId, mockAccountsRegistry)
        ).toBe(account)
      }
    )
  })
})

describe('Test getAccountTypeDescription', () => {
  test('ETH Account Description', () => {
    expect(getAccountTypeDescription(mockEthAccountInfo.accountId)).toEqual(
      'braveWalletETHAccountDescription'
    )
  })
  test('SOL Account Description', () => {
    expect(getAccountTypeDescription(mockSolanaAccount.accountId)).toEqual(
      'braveWalletSOLAccountDescription'
    )
  })
  test('FIL Account Description', () => {
    expect(getAccountTypeDescription(mockFilecoinAccount.accountId)).toEqual(
      'braveWalletFILAccountDescription'
    )
  })
  test('BTC Mainnet Account Description', () => {
    expect(getAccountTypeDescription(mockBitcoinAccount.accountId)).toEqual(
      'braveWalletBTCMainnetAccountDescription'
    )
  })
  test('BTC Testnet Account Description', () => {
    expect(
      getAccountTypeDescription(mockBitcoinTestnetAccount.accountId)
    ).toEqual('braveWalletBTCTestnetAccountDescription')
  })
})
