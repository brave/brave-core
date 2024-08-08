// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// types
import {
  SupportedTestNetworks,
  SupportedCoinTypes,
  BraveWallet,
  PanelTypes,
  AssetIdsByCollectionNameRegistry
} from '../constants/types'
import {
  TokenBalancesRegistry //
} from '../common/slices/entities/token-balance.entity'

// utils
import { getNetworkId } from '../common/slices/entities/network.entity'
import { LOCAL_STORAGE_KEYS } from '../common/constants/local-storage-keys'
import { createEmptyTokenBalancesRegistry } from './balance-utils'

/** Set local storage in a way that hooks can detect the change */
export const setLocalStorageItem = (key: string, stringifiedValue: string) => {
  window.localStorage.setItem(key, stringifiedValue)
  window.dispatchEvent(
    new StorageEvent('local-storage', {
      key
    })
  )
}

export const parseJSONFromLocalStorage = <T = any>(
  storageString: keyof typeof LOCAL_STORAGE_KEYS,
  fallback: T
): T => {
  try {
    return JSON.parse(
      window.localStorage.getItem(LOCAL_STORAGE_KEYS[storageString]) || ''
    ) as T
  } catch (e) {
    return fallback
  }
}

export const makeInitialFilteredOutNetworkKeys = () => {
  const localHostNetworkKeys = SupportedCoinTypes.map((coin) => {
    return getNetworkId({
      chainId: BraveWallet.LOCALHOST_CHAIN_ID,
      coin: coin
    })
  })
  const testNetworkKeys = SupportedTestNetworks.filter(
    (chainId) => chainId !== BraveWallet.LOCALHOST_CHAIN_ID
  ).map((chainId) => {
    if (
      chainId === BraveWallet.SOLANA_DEVNET ||
      chainId === BraveWallet.SOLANA_TESTNET
    ) {
      return getNetworkId({
        chainId: chainId,
        coin: BraveWallet.CoinType.SOL
      })
    }
    if (chainId === BraveWallet.FILECOIN_TESTNET) {
      return getNetworkId({
        chainId: chainId,
        coin: BraveWallet.CoinType.FIL
      })
    }
    return getNetworkId({
      chainId: chainId,
      coin: BraveWallet.CoinType.ETH
    })
  })
  return [...testNetworkKeys, ...localHostNetworkKeys]
}

export function isPersistanceOfPanelProhibited(panelType: PanelTypes) {
  return (
    panelType === 'connectWithSite' || panelType === 'connectHardwareWallet'
  )
}

export function storeCurrentAndPreviousPanel(
  panelType: PanelTypes,
  previousPanel: PanelTypes | undefined
) {
  if (!isPersistanceOfPanelProhibited(panelType)) {
    window.localStorage.setItem(LOCAL_STORAGE_KEYS.CURRENT_PANEL, panelType)
  }

  if (previousPanel && !isPersistanceOfPanelProhibited(previousPanel)) {
    window.localStorage.setItem(
      LOCAL_STORAGE_KEYS.LAST_VISITED_PANEL,
      previousPanel
    )
  }
}

export function getStoredPortfolioTimeframe(): BraveWallet.AssetPriceTimeframe {
  const storedValue = window.localStorage.getItem(
    LOCAL_STORAGE_KEYS.PORTFOLIO_TIME_LINE_OPTION
  )

  if (storedValue !== undefined) {
    return Number(
      window.localStorage.getItem(LOCAL_STORAGE_KEYS.PORTFOLIO_TIME_LINE_OPTION)
    )
  }

  return BraveWallet.AssetPriceTimeframe.OneDay
}

export function setStoredPortfolioTimeframe(
  timeframe: BraveWallet.AssetPriceTimeframe
) {
  window.localStorage.setItem(
    LOCAL_STORAGE_KEYS.PORTFOLIO_TIME_LINE_OPTION,
    timeframe.toString()
  )
}

export const getPersistedTokenBalances = (
  isSpamRegistry = false
): TokenBalancesRegistry => {
  try {
    const registry: TokenBalancesRegistry = JSON.parse(
      window.localStorage.getItem(
        isSpamRegistry
          ? LOCAL_STORAGE_KEYS.SPAM_TOKEN_BALANCES
          : LOCAL_STORAGE_KEYS.TOKEN_BALANCES
      ) || JSON.stringify(createEmptyTokenBalancesRegistry())
    )
    if (registry.accounts) {
      return registry
    }
    return createEmptyTokenBalancesRegistry()
  } catch (error) {
    console.error(error)
    return createEmptyTokenBalancesRegistry()
  }
}

export const getPersistedTokenBalancesSubset = (arg: {
  accountIds: BraveWallet.AccountId[]
  networks: Array<Pick<BraveWallet.NetworkInfo, 'chainId' | 'coin'>>
  isSpamRegistry?: boolean
}): TokenBalancesRegistry | null => {
  const persistedBalances = getPersistedTokenBalances(arg.isSpamRegistry)

  // return a subset of the registry for
  // the accounts and networks passed as args
  const registrySubset: TokenBalancesRegistry = {
    accounts: {}
  }

  for (const accountId of arg.accountIds) {
    // only return info for accounts that have been passed as args
    if (persistedBalances.accounts[accountId.uniqueKey]) {
      registrySubset.accounts[accountId.uniqueKey] =
        persistedBalances.accounts[accountId.uniqueKey]
      // filter account balances to just
      // the chains that have been passed as args
      const accountNetworkIds = Object.keys(
        registrySubset.accounts[accountId.uniqueKey].chains
      )
      for (const network of arg.networks) {
        const networkId = getNetworkId(network)
        if (!accountNetworkIds.includes(networkId)) {
          delete registrySubset.accounts[accountId.uniqueKey].chains[networkId]
        }
      }
    }
  }

  // return null if the subset is empty
  return Object.keys(registrySubset.accounts).length ? registrySubset : null
}

export const setPersistedPortfolioTokenBalances = (
  registry: TokenBalancesRegistry,
  isSpamRegistry = false
) => {
  try {
    window.localStorage.setItem(
      isSpamRegistry
        ? LOCAL_STORAGE_KEYS.SPAM_TOKEN_BALANCES
        : LOCAL_STORAGE_KEYS.TOKEN_BALANCES,
      JSON.stringify(registry)
    )
  } catch (error) {
    console.error(error)
  }
}

export const getPersistedNftCollectionNamesRegistry =
  (): AssetIdsByCollectionNameRegistry => {
    const emptyRegistry: AssetIdsByCollectionNameRegistry = {}
    try {
      const registry: AssetIdsByCollectionNameRegistry = JSON.parse(
        window.localStorage.getItem(
          LOCAL_STORAGE_KEYS.NFT_COLLECTION_NAMES_REGISTRY
        ) || JSON.stringify(emptyRegistry)
      )
      return registry ?? emptyRegistry
    } catch (error) {
      console.error(error)
      return emptyRegistry
    }
  }

export const setPersistedNftCollectionNamesRegistry = (
  registry: AssetIdsByCollectionNameRegistry
) => {
  try {
    window.localStorage.setItem(
      LOCAL_STORAGE_KEYS.NFT_COLLECTION_NAMES_REGISTRY,
      JSON.stringify(registry)
    )
  } catch (error) {
    console.error(error)
  }
}
