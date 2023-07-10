// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { assertNotReached } from 'chrome://resources/js/assert_ts.js';
import { getLocale } from '../../common/locale'

// types
import {
  BraveWallet,
  WalletAccountType,
  WalletAccountTypeName
} from '../constants/types'

// constants
import registry from '../common/constants/registry'

// utils
import { reduceAddress } from './reduce-address'
import { EntityState } from '@reduxjs/toolkit'
import { AccountInfoEntity } from '../common/slices/entities/account-info.entity'

export const sortAccountsByName = (accounts: WalletAccountType[]) => {
  return [...accounts].sort(function (a: WalletAccountType, b: WalletAccountType) {
    if (a.name < b.name) {
      return -1
    }

    if (a.name > b.name) {
      return 1
    }

    return 0
  })
}

export const groupAccountsById = (accounts: WalletAccountType[], key: string) => {
  return accounts.reduce<Record<string, WalletAccountType[]>>((result, obj) => {
    (result[obj[key]] = result[obj[key]] || []).push(obj)
    return result
  }, {})
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
  return accounts.find(
    (account) => account.address.toLowerCase() === address.toLowerCase()
  )?.name
}

export const createTokenBalanceRegistryKey = (
  token: Pick<
    BraveWallet.BlockchainToken,
    | 'tokenId'
    | 'isErc721'
    | 'contractAddress'
  >
) => {
  return token.isErc721 ? `${token.contractAddress.toLowerCase()}#${token.tokenId}` : token.contractAddress.toLowerCase()
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

export const findAccountFromRegistry = (
  address: string,
  accounts: EntityState<AccountInfoEntity>
): AccountInfoEntity | undefined => {
  return accounts.entities[address]
}

export const findAccountFromRegistryByAccountId = (
  accountId: BraveWallet.AccountId,
  accounts: EntityState<AccountInfoEntity>
): AccountInfoEntity | undefined => {
  // TODO(apaymyshev): should be indexed by uniqueKey
  return accounts.entities[accountId.address]
}

export const getAddressLabelFromRegistry = (
  address: string,
  accounts: EntityState<AccountInfoEntity>
): string => {
  return (
    registry[address.toLowerCase()] ??
    accounts.entities[address]?.name ??
    reduceAddress(address)
  )
}

export const getAccountId = (account: { address: string }) => {
  // TODO(apaymyshev): should use uniuqeKey
  return account.address
}

export function isHardwareAccount(
  account: Pick<BraveWallet.AccountInfo, 'hardware'>
) {
  return !!account.hardware?.deviceId
}

export const keyringIdForNewAccount = (
  coin: BraveWallet.CoinType,
  network?: string | undefined
) => {
  if (coin === BraveWallet.CoinType.ETH) {
    return BraveWallet.KeyringId.kDefault
  }

  if (coin === BraveWallet.CoinType.SOL) {
    return BraveWallet.KeyringId.kSolana
  }

  if (
    coin === BraveWallet.CoinType.FIL &&
    network === BraveWallet.FILECOIN_MAINNET
  ) {
    return BraveWallet.KeyringId.kFilecoin
  }

  if (
    coin === BraveWallet.CoinType.FIL &&
    network === BraveWallet.FILECOIN_TESTNET
  ) {
    return BraveWallet.KeyringId.kFilecoinTestnet
  }

  assertNotReached()
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
      return ''
  }
}
