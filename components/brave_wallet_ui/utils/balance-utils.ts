// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// types
import { TokenBalancesRegistry } from '../common/slices/entities/token-balance.entity'
import { BraveWallet, SupportedTestNetworks } from '../constants/types'

// utils
import Amount from './amount'

export const formatTokenBalanceWithSymbol = (
  balance: string,
  decimals: number,
  symbol: string,
  decimalPlace?: number
) => {
  return `${new Amount(balance)
    .divideByDecimals(decimals)
    .format(decimalPlace ?? 6, true)} ${symbol}`
}

export const getAccountBalancesKey = (
  accountId: Pick<BraveWallet.AccountId, 'uniqueKey'>
) => {
  return accountId.uniqueKey
}

export const getPercentAmount = (
  asset: BraveWallet.BlockchainToken,
  accountId: BraveWallet.AccountId,
  percent: number,
  tokenBalancesRegistry: TokenBalancesRegistry | undefined | null
): string => {
  const assetBalance =
    getBalance(accountId, asset, tokenBalancesRegistry) || '0'
  const amountWrapped = new Amount(assetBalance).times(percent).parseInteger()

  const formattedAmount =
    percent === 1
      ? amountWrapped.divideByDecimals(asset.decimals).format()
      : amountWrapped.divideByDecimals(asset.decimals).format(6)

  return formattedAmount
}

export const getBalance = (
  accountId: BraveWallet.AccountId | undefined,
  asset: BraveWallet.BlockchainToken | undefined,
  tokenBalancesRegistry: TokenBalancesRegistry | undefined | null
) => {
  if (!accountId || !asset || !tokenBalancesRegistry) {
    return ''
  }

  const accountBalances =
    tokenBalancesRegistry.accounts[getAccountBalancesKey(accountId)]
  if (!accountBalances) {
    return '0'
  }

  const chainIdBalances = accountBalances.chains[asset.chainId]
  if (!chainIdBalances) {
    return '0'
  }

  const balance =
    chainIdBalances.tokenBalances[asset.contractAddress.toLowerCase()]
  return balance || '0'
}

export function setBalance(
  accountId: Pick<BraveWallet.AccountId, 'uniqueKey'>,
  chainId: string,
  contractAddress: string,
  balance: string,
  tokenBalancesRegistry: TokenBalancesRegistry
) {
  const accountBalanceKey = getAccountBalancesKey(accountId)
  if (!tokenBalancesRegistry.accounts[accountBalanceKey]) {
    tokenBalancesRegistry.accounts[accountBalanceKey] = { chains: {} }
  }

  const accountBalances = tokenBalancesRegistry.accounts[accountBalanceKey]
  if (!accountBalances.chains[chainId]) {
    accountBalances.chains[chainId] = { tokenBalances: {} }
  }

  const chainBalances = accountBalances.chains[chainId]
  chainBalances.tokenBalances[contractAddress.toLowerCase()] = balance
}

export function createEmptyTokenBalancesRegistry(): TokenBalancesRegistry {
  return {
    accounts: {}
  }
}

export const getActiveWalletCount = (
  accounts: BraveWallet.AccountInfo[],
  tokenBalancesRegistry: TokenBalancesRegistry,
  countTestNetworks: boolean
) => {
  const activeWalletCount: Record<BraveWallet.CoinType, number> = {}

  accounts.map((account) => {
    const accountBalances =
      tokenBalancesRegistry.accounts[getAccountBalancesKey(account.accountId)]
    if (!accountBalances) {
      return
    }

    const coin = account.accountId.coin

    if (activeWalletCount[coin] === undefined) {
      activeWalletCount[coin] = 0
    }

    let chainsWithBalance: string[] = []

    for (const [chainId, chainBalances] of Object.entries(
      accountBalances.chains
    )) {
      for (const tokenBalance of Object.values(chainBalances.tokenBalances)) {
        const amount = new Amount(tokenBalance)
        if (amount && amount.gt('0')) {
          chainsWithBalance = [...chainsWithBalance, chainId]
          break
        }
      }
    }

    const hasMainnetBalance = chainsWithBalance.some(
      (chainId) => !SupportedTestNetworks.includes(chainId)
    )
    const hasTestnetBalance = chainsWithBalance.some((chainId) =>
      SupportedTestNetworks.includes(chainId)
    )

    if (hasMainnetBalance || (countTestNetworks && hasTestnetBalance)) {
      activeWalletCount[coin] += 1
    }
  })

  return activeWalletCount
}
