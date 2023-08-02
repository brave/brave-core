// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  BraveWallet,
  CoinType,
  SupportedTestNetworks,
  SupportedCoinTypes
} from '../constants/types'
import {
  networkEntityAdapter
} from '../common/slices/entities/network.entity'
import { LOCAL_STORAGE_KEYS } from '../common/constants/local-storage-keys'

export const parseJSONFromLocalStorage = <T = any> (
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
    return networkEntityAdapter.selectId(
      {
        chainId: BraveWallet.LOCALHOST_CHAIN_ID,
        coin: coin
      }
    ).toString()
  })
  const testNetworkKeys = SupportedTestNetworks
    .filter((chainId) => chainId !== BraveWallet.LOCALHOST_CHAIN_ID)
    .map((chainId) => {
      if (
        chainId === BraveWallet.SOLANA_DEVNET ||
        chainId === BraveWallet.SOLANA_TESTNET
      ) {
        return networkEntityAdapter.selectId(
          {
            chainId: chainId,
            coin: CoinType.SOL
          }
        ).toString()
      }
      if (chainId === BraveWallet.FILECOIN_TESTNET) {
        return networkEntityAdapter.selectId(
          {
            chainId: chainId,
            coin: CoinType.FIL
          }
        ).toString()
      }
      return networkEntityAdapter.selectId(
        {
          chainId: chainId,
          coin: CoinType.ETH
        }
      ).toString()
    })
  return [...testNetworkKeys, ...localHostNetworkKeys]
}
