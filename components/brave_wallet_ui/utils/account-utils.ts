// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { assertNotReached } from 'chrome://resources/js/assert.js'
import { getLocale } from '../../common/locale'

// types
import {
  BraveWallet,
  BitcoinMainnetKeyringIds,
  BitcoinTestnetKeyringIds
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

export const entityIdFromAccountId = (
  accountId: Pick<BraveWallet.AccountId, 'uniqueKey'>
) => {
  return accountId.uniqueKey
}

export const findAccountByAddress = (
  address: string,
  accounts: EntityState<BraveWallet.AccountInfo> | undefined
): BraveWallet.AccountInfo | undefined => {
  if (!address || !accounts) return undefined
  for (const id of accounts.ids) {
    if (
      accounts.entities[id]?.address.toLowerCase() === address.toLowerCase()
    ) {
      return accounts.entities[id]
    }
  }
  return undefined
}

export const findAccountByAccountId = (
  accountId: Pick<BraveWallet.AccountId, 'uniqueKey'>,
  accounts: EntityState<BraveWallet.AccountInfo> | undefined
): BraveWallet.AccountInfo | undefined => {
  if (!accounts) {
    return undefined
  }

  return accounts.entities[entityIdFromAccountId(accountId)]
}

export const getAddressLabel = (
  address: string,
  accounts?: EntityState<BraveWallet.AccountInfo>
): string => {
  if (!accounts) {
    return registry[address.toLowerCase()] ?? reduceAddress(address)
  }
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
    if (
      chainId === BraveWallet.FILECOIN_MAINNET ||
      chainId === BraveWallet.LOCALHOST_CHAIN_ID
    ) {
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

  if (coin === BraveWallet.CoinType.ZEC) {
    if (chainId === BraveWallet.Z_CASH_MAINNET) {
      return BraveWallet.KeyringId.kZCashMainnet
    }
    if (chainId === BraveWallet.Z_CASH_TESTNET) {
      return BraveWallet.KeyringId.kZCashTestnet
    }
  }

  assertNotReached(`Unknown coin ${coin} and chainId ${chainId}`)
}

const zcashTestnetKeyrings = [BraveWallet.KeyringId.kZCashTestnet]

export const getAccountTypeDescription = (accountId: BraveWallet.AccountId) => {
  switch (accountId.coin) {
    case BraveWallet.CoinType.ETH:
      return getLocale('braveWalletETHAccountDescription')
    case BraveWallet.CoinType.SOL:
      return getLocale('braveWalletSOLAccountDescription')
    case BraveWallet.CoinType.FIL:
      return getLocale('braveWalletFILAccountDescription')
    case BraveWallet.CoinType.BTC:
      if (BitcoinTestnetKeyringIds.includes(accountId.keyringId)) {
        return getLocale('braveWalletBTCTestnetAccountDescription')
      }
      return getLocale('braveWalletBTCMainnetAccountDescription')
    case BraveWallet.CoinType.ZEC:
      if (zcashTestnetKeyrings.includes(accountId.keyringId)) {
        return getLocale('braveWalletZECTestnetAccountDescription')
      }
      return getLocale('braveWalletZECAccountDescription')
    default:
      assertNotReached(`Unknown coin ${accountId.coin}`)
  }
}

export const isFVMAccount = (
  account: BraveWallet.AccountInfo,
  network: BraveWallet.NetworkInfo
) => {
  return (
    (network.chainId === BraveWallet.FILECOIN_ETHEREUM_MAINNET_CHAIN_ID &&
      account.accountId.keyringId === BraveWallet.KeyringId.kFilecoin) ||
    (network.chainId === BraveWallet.FILECOIN_ETHEREUM_TESTNET_CHAIN_ID &&
      account.accountId.keyringId === BraveWallet.KeyringId.kFilecoinTestnet)
  )
}

export const getAccountsForNetwork = (
  network: Pick<BraveWallet.NetworkInfo, 'chainId' | 'coin'>,
  accounts: BraveWallet.AccountInfo[]
) => {
  if (network.chainId === BraveWallet.BITCOIN_MAINNET) {
    return accounts.filter((account) =>
      BitcoinMainnetKeyringIds.includes(account.accountId.keyringId)
    )
  }
  if (network.chainId === BraveWallet.BITCOIN_TESTNET) {
    return accounts.filter((account) =>
      BitcoinTestnetKeyringIds.includes(account.accountId.keyringId)
    )
  }
  if (network.chainId === BraveWallet.Z_CASH_MAINNET) {
    return accounts.filter(
      (account) =>
        account.accountId.keyringId === BraveWallet.KeyringId.kZCashMainnet
    )
  }
  if (network.chainId === BraveWallet.Z_CASH_TESTNET) {
    return accounts.filter(
      (account) =>
        account.accountId.keyringId === BraveWallet.KeyringId.kZCashTestnet
    )
  }
  if (network.chainId === BraveWallet.FILECOIN_MAINNET) {
    return accounts.filter(
      (account) =>
        account.accountId.keyringId === BraveWallet.KeyringId.kFilecoin
    )
  }
  if (network.chainId === BraveWallet.FILECOIN_TESTNET) {
    return accounts.filter(
      (account) =>
        account.accountId.keyringId === BraveWallet.KeyringId.kFilecoinTestnet
    )
  }
  return accounts.filter((account) => account.accountId.coin === network.coin)
}
