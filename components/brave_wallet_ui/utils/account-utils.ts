// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { assertNotReached } from 'chrome://resources/js/assert_ts.js';
import { getLocale } from '../../common/locale'

// types
import {
  BraveWallet,
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

export const entityIdFromAccountId = (
  accountId: Pick<BraveWallet.AccountId, 'address' | 'uniqueKey'>
) => {
  // TODO(apaymyshev): should use uniqueKey always
  return accountId.address || accountId.uniqueKey
}

export const findAccountByAddress = (
  address: string,
  accounts: EntityState<BraveWallet.AccountInfo> | undefined
): BraveWallet.AccountInfo | undefined => {
  if (!address || ! accounts)
    return undefined
  for (const id of accounts.ids) {
    if (accounts.entities[id]?.address.toLowerCase() === address.toLowerCase()) {
      return accounts.entities[id]
    }
  }
  return undefined
}

export const findAccountByAccountId = (
  accountId: BraveWallet.AccountId,
  accounts: EntityState<BraveWallet.AccountInfo> | undefined
): BraveWallet.AccountInfo | undefined => {
  if (!accounts) {
    return undefined
  }
  return accounts.entities[entityIdFromAccountId(accountId)]
}

export const getAddressLabel = (
  address: string,
  accounts: EntityState<BraveWallet.AccountInfo>
): string => {
  return (
    registry[address.toLowerCase()] ??
    findAccountByAddress(address, accounts)?.name ??
    reduceAddress(address)
  )
}

export const getAccountLabel = (
  accountId: BraveWallet.AccountId,
  accounts: EntityState<BraveWallet.AccountInfo>
): string => {
  return (
    findAccountByAccountId(accountId, accounts)?.name ??
    reduceAddress(accountId.address)
  )
}

export function isHardwareAccount(account: BraveWallet.AccountId) {
  return account.kind === BraveWallet.AccountKind.kHardware
}

export const keyringIdForNewAccount = (
  coin: BraveWallet.CoinType,
  chainId?: string | undefined
) => {
  if (coin === BraveWallet.CoinType.ETH) {
    return BraveWallet.KeyringId.kDefault
  }

  if (coin === BraveWallet.CoinType.SOL) {
    return BraveWallet.KeyringId.kSolana
  }

  if (coin === BraveWallet.CoinType.FIL) {
    if (chainId === BraveWallet.FILECOIN_MAINNET) {
      return BraveWallet.KeyringId.kFilecoin
    }
    if (chainId === BraveWallet.FILECOIN_TESTNET) {
      return BraveWallet.KeyringId.kFilecoinTestnet
    }
  }

  if (coin === BraveWallet.CoinType.BTC) {
    if (chainId === BraveWallet.BITCOIN_MAINNET) {
      return BraveWallet.KeyringId.kBitcoin84
    }
    if (chainId === BraveWallet.BITCOIN_TESTNET) {
      return BraveWallet.KeyringId.kBitcoin84Testnet
    }
  }

  assertNotReached(`Unknown coin ${coin} and chainId ${chainId}`)
}

export const getAccountTypeDescription = (coin: BraveWallet.CoinType) => {
  switch (coin) {
    case BraveWallet.CoinType.ETH:
      return getLocale('braveWalletETHAccountDescrption')
    case BraveWallet.CoinType.SOL:
      return getLocale('braveWalletSOLAccountDescrption')
    case BraveWallet.CoinType.FIL:
      return getLocale('braveWalletFILAccountDescrption')
    case BraveWallet.CoinType.BTC:
      return getLocale('braveWalletBTCAccountDescrption')
    default:
      assertNotReached(`Unknown coin ${coin}`)
  }
}
