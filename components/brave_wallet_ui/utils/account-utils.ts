// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// types
import { AccountInfo, BraveWallet, WalletAccountType } from '../constants/types'

// constants
import registry from '../common/constants/registry'

// utils
import { reduceAddress } from './reduce-address'

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

export const findAccountByAddress = (accounts: WalletAccountType[], address: string): WalletAccountType | undefined => {
  return accounts.find((account) => address === account.address)
}

export const findAccountName = (accounts: WalletAccountType[], address: string) => {
  return accounts.find((account) => account.address.toLowerCase() === address.toLowerCase())?.name
}

export const createTokenBalanceRegistryKey = (token: BraveWallet.BlockchainToken) => {
  return token.isErc721 ? `${token.contractAddress.toLowerCase()}#${token.tokenId}` : token.contractAddress.toLowerCase()
}

export const getAccountType = (info: AccountInfo) => {
  if (info.hardware) {
    return info.hardware.vendor
  }
  return info.isImported ? 'Secondary' : 'Primary'
}

export const getAddressLabel = (address: string, accounts: WalletAccountType[]): string => {
  return (
    registry[address.toLowerCase()] ??
    findAccountName(accounts, address) ??
    reduceAddress(address)
  )
}
