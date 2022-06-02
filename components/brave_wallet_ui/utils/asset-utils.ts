/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as BraveWallet from 'gen/brave/components/brave_wallet/common/brave_wallet.mojom.m.js'
import { getRampNetworkPrefix } from './string-utils'

export const getUniqueAssets = (assets: BraveWallet.BlockchainToken[]) => {
  return assets.filter((asset, index) => {
    return index === assets.findIndex(item => {
      return item.contractAddress.toLowerCase() === asset.contractAddress.toLowerCase() && item.chainId === asset.chainId
    })
  })
}

export const isSelectedAssetInAssetOptions = (selectedAsset: BraveWallet.BlockchainToken, assetOptions: BraveWallet.BlockchainToken[]) => {
  return assetOptions.findIndex(asset => {
    return asset.contractAddress.toLowerCase() === selectedAsset?.contractAddress.toLowerCase() &&
      asset.chainId === selectedAsset.chainId &&
      asset.symbol.toLowerCase() === selectedAsset.symbol.toLowerCase()
  }) !== -1
}

export const getRampAssetSymbol = (asset: BraveWallet.BlockchainToken) => {
  if (asset.symbol.toUpperCase() === 'BAT' && asset.chainId === BraveWallet.MAINNET_CHAIN_ID) {
    // BAT is the only token on Ethereum Mainnet with a prefix on Ramp.Network
    return 'ETH_BAT'
  }

  const rampNetworkPrefix = getRampNetworkPrefix(asset.chainId)
  return rampNetworkPrefix !== '' ? `${rampNetworkPrefix}_${asset.symbol.toUpperCase()}` : asset.symbol
}
