// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// types
import {
  SupportedTestNetworks,
  SupportedCoinTypes,
  BraveWallet,
  PanelTypes
} from '../constants/types'
import {
  TokenBalancesRegistry //
} from '../common/slices/entities/token-balance.entity'

// utils
import { networkEntityAdapter } from '../common/slices/entities/network.entity'
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
    return networkEntityAdapter
      .selectId({
        chainId: BraveWallet.LOCALHOST_CHAIN_ID,
        coin: coin
      })
      .toString()
  })
  const testNetworkKeys = SupportedTestNetworks.filter(
    (chainId) => chainId !== BraveWallet.LOCALHOST_CHAIN_ID
  ).map((chainId) => {
    if (
      chainId === BraveWallet.SOLANA_DEVNET ||
      chainId === BraveWallet.SOLANA_TESTNET
    ) {
      return networkEntityAdapter
        .selectId({
          chainId: chainId,
          coin: BraveWallet.CoinType.SOL
        })
        .toString()
    }
    if (chainId === BraveWallet.FILECOIN_TESTNET) {
      return networkEntityAdapter
        .selectId({
          chainId: chainId,
          coin: BraveWallet.CoinType.FIL
        })
        .toString()
    }
    return networkEntityAdapter
      .selectId({
        chainId: chainId,
        coin: BraveWallet.CoinType.ETH
      })
      .toString()
  })
  return [...testNetworkKeys, ...localHostNetworkKeys]
}

export function isPersistanceOfPanelProhibited(panelType: PanelTypes) {
  return (
    panelType === 'connectWithSite' ||
    panelType === 'connectHardwareWallet'
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

export const getPersistedPortfolioTokenBalances = (): TokenBalancesRegistry => {
  try {
    return JSON.parse(
      window.localStorage.getItem(LOCAL_STORAGE_KEYS.TOKEN_BALANCES) || '{}'
    )
  } catch (error) {
    console.error(error)
    return createEmptyTokenBalancesRegistry()
  }
}

export const setPersistedPortfolioTokenBalances = (
  registry: TokenBalancesRegistry
) => {
  try {
    window.localStorage.setItem(
      LOCAL_STORAGE_KEYS.TOKEN_BALANCES,
      JSON.stringify(registry)
    )
  } catch (error) {
    console.error(error)
  }
}
