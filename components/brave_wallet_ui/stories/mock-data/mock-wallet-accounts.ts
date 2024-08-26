// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import {
  AccountFromDevice,
  EthLedgerLiveHardwareImportScheme
} from '../../common/hardware/types'
import { BraveWallet } from '../../constants/types'

export const mockEthAccount = {
  name: 'Account 1',
  address: '0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14',
  nativeBalanceRegistry: {
    '0x1': '311780000000000000'
  },
  tokenBalanceRegistry: {},
  accountId: {
    coin: BraveWallet.CoinType.ETH,
    keyringId: BraveWallet.KeyringId.kDefault,
    kind: BraveWallet.AccountKind.kDerived,
    address: '0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14',
    accountIndex: 0,
    uniqueKey: 'mockEthAccount_uniqueKey'
  },
  hardware: undefined
}

export const mockBitcoinAccount = {
  address: '',
  accountId: {
    coin: 0,
    keyringId: BraveWallet.KeyringId.kBitcoin84,
    kind: BraveWallet.AccountKind.kDerived,
    address: '',
    accountIndex: 0,
    uniqueKey: 'mockBitcoinAccount_uniqueKey'
  },
  name: 'Bitcoin Account',
  tokenBalanceRegistry: {},
  nativeBalanceRegistry: {
    [BraveWallet.BITCOIN_MAINNET]: '123456789'
  },
  hardware: undefined
}

export const mockAccounts: BraveWallet.AccountInfo[] = [
  mockEthAccount,
  {
    name: 'Account 2',
    address: '0x73A29A1da97149722eB09c526E4eAd698895bDCf',
    accountId: {
      coin: BraveWallet.CoinType.ETH,
      keyringId: BraveWallet.KeyringId.kDefault,
      kind: BraveWallet.AccountKind.kDerived,
      address: '0x73A29A1da97149722eB09c526E4eAd698895bDCf',
      accountIndex: 0,
      uniqueKey: '2'
    },
    hardware: undefined
  },
  {
    name: 'Account 3',
    address: '0x3f29A1da97149722eB09c526E4eAd698895b426',
    accountId: {
      coin: BraveWallet.CoinType.ETH,
      keyringId: BraveWallet.KeyringId.kDefault,
      kind: BraveWallet.AccountKind.kDerived,
      address: '0x3f29A1da97149722eB09c526E4eAd698895b426',
      accountIndex: 0,
      uniqueKey: '3'
    },
    hardware: undefined
  },
  {
    address: '9RaoGw6VQM1SFgX8wtfUL1acv5uuLNaySELJV2orEZbN',
    accountId: {
      coin: 501,
      keyringId: BraveWallet.KeyringId.kSolana,
      kind: BraveWallet.AccountKind.kDerived,
      address: '9RaoGw6VQM1SFgX8wtfUL1acv5uuLNaySELJV2orEZbN',
      accountIndex: 0,
      uniqueKey: '9RaoGw6VQM1SFgX8wtfUL1acv5uuLNaySELJV2orEZbN'
    },
    name: 'Solana Account',
    hardware: undefined
  },
  mockBitcoinAccount
]

export const mockAccountsFromDevice: AccountFromDevice[] = [
  {
    address: '0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14',
    derivationPath: EthLedgerLiveHardwareImportScheme.pathTemplate(0)
  },
  {
    address: '0x73A29A1da97149722eB09c526E4eAd698895bDCf',
    derivationPath: EthLedgerLiveHardwareImportScheme.pathTemplate(1)
  }
]

export const mockedTransactionAccounts: BraveWallet.AccountInfo[] = [
  {
    name: 'Account 1',
    address: '1',
    accountId: {
      coin: BraveWallet.CoinType.ETH,
      keyringId: BraveWallet.KeyringId.kDefault,
      kind: BraveWallet.AccountKind.kDerived,
      address: '1',
      accountIndex: 0,
      uniqueKey: '1'
    },
    hardware: undefined
  }
]
