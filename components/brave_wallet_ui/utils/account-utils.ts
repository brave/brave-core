// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { assertNotReached } from 'chrome://resources/js/assert_ts.js';
import { getLocale } from '../../common/locale'

// types
import {
  BraveWallet,
  CoinType,
  WalletAccountTypeName
} from '../constants/types'

// constants
import registry from '../common/constants/registry'

// utils
import { reduceAddress } from './reduce-address'
import { EntityState } from '@reduxjs/toolkit'

export const sortAccountsByName = (accounts: BraveWallet.AccountInfo[]) => {
  return [...accounts].sort(function (
    a: BraveWallet.AccountInfo,
    b: BraveWallet.AccountInfo
  ) {
    if (a.name < b.name) {
      return -1
    }

    if (a.name > b.name) {
      return 1
    }

    return 0
  })
}

export const groupAccountsById = (
  accounts: BraveWallet.AccountInfo[],
  key: string
) => {
  return accounts.reduce<Record<string, BraveWallet.AccountInfo[]>>(
    (result, obj) => {
      ;(result[obj[key]] = result[obj[key]] || []).push(obj)
      return result
    },
    {}
  )
}

export const findAccountByAddress = <T extends { address: string }>(
  accounts: T[],
  address: string
): T | undefined => {
  return accounts.find((account) => address === account.address)
}

export const findAccountByUniqueKey = <
  T extends { accountId: { uniqueKey: string } }
>(
  accounts: T[],
  uniqueKey: string | undefined
): T | undefined => {
  if (!uniqueKey) {
    return
  }

  return accounts.find((account) => uniqueKey === account.accountId.uniqueKey)
}

export const findAccountByAccountId = <
  T extends { accountId: { uniqueKey: string } }
>(
  accounts: T[],
  accountId: BraveWallet.AccountId | undefined
): T | undefined => {
  if (!accountId) {
    return
  }

  return findAccountByUniqueKey(accounts, accountId.uniqueKey)
}

export const findAccountName = <
  T extends {
    address: string
    name: string
  }
>(
  accounts: T[],
  address: string
) => {
  if (!address) {
    return undefined
  }
  return accounts.find(
    (account) => account.address.toLowerCase() === address.toLowerCase()
  )?.name
}

export const getAccountType = (
  info: Pick<BraveWallet.AccountInfo, 'accountId' | 'hardware'>
): WalletAccountTypeName => {
  if (info.accountId.kind === BraveWallet.AccountKind.kHardware) {
    return info.hardware!.vendor as 'Ledger' | 'Trezor'
  }
  return info.accountId.kind === BraveWallet.AccountKind.kImported
    ? 'Secondary'
    : 'Primary'
}

export const getAddressLabel = <
  T extends Array<{
    address: string
    name: string
  }>
>(
  address: string,
  accounts: T
): string => {
  return (
    registry[address.toLowerCase()] ??
    findAccountName(accounts, address) ??
    reduceAddress(address)
  )
}

export const entityIdFromAccountId = (
  accountId: Pick<BraveWallet.AccountId, 'address' | 'uniqueKey'>
) => {
  // TODO(apaymyshev): should use uniqueKey always
  return accountId.address || accountId.uniqueKey
}

export const findAccountFromRegistry = (
  address: string,
  accounts: EntityState<BraveWallet.AccountInfo>
): BraveWallet.AccountInfo | undefined => {
  return accounts.entities[entityIdFromAccountId({ address, uniqueKey: '' })]
}

export const getAddressLabelFromRegistry = (
  address: string,
  accounts: EntityState<BraveWallet.AccountInfo>
): string => {
  return (
    registry[address.toLowerCase()] ??
    accounts.entities[address]?.name ??
    reduceAddress(address)
  )
}

export function isHardwareAccount(
  account: Pick<BraveWallet.AccountInfo, 'hardware'>
) {
  return !!account.hardware?.deviceId
}

export const keyringIdForNewAccount = (
  coin: CoinType,
  chainId?: string | undefined
) => {
  if (coin === CoinType.ETH) {
    return BraveWallet.KeyringId.kDefault
  }

  if (coin === CoinType.SOL) {
    return BraveWallet.KeyringId.kSolana
  }

  if (coin === CoinType.FIL) {
    if (chainId === BraveWallet.FILECOIN_MAINNET) {
      return BraveWallet.KeyringId.kFilecoin
    }
    if (chainId === BraveWallet.FILECOIN_TESTNET) {
      return BraveWallet.KeyringId.kFilecoinTestnet
    }
  }

  if (coin === CoinType.BTC) {
    if (chainId === BraveWallet.BITCOIN_MAINNET) {
      return BraveWallet.KeyringId.kBitcoin84
    }
    if (chainId === BraveWallet.BITCOIN_TESTNET) {
      return BraveWallet.KeyringId.kBitcoin84Testnet
    }
  }

  assertNotReached(`Unknown coin ${coin} and chainId ${chainId}`)
}

export const getAccountTypeDescription = (coin: CoinType) => {
  switch (coin) {
    case CoinType.ETH:
      return getLocale('braveWalletETHAccountDescrption')
    case CoinType.SOL:
      return getLocale('braveWalletSOLAccountDescrption')
    case CoinType.FIL:
      return getLocale('braveWalletFILAccountDescrption')
    case CoinType.BTC:
      return getLocale('braveWalletBTCAccountDescrption')
    default:
      assertNotReached(`Unknown coin ${coin}`)
  }
}
