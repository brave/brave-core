// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { mapLimit } from 'async'
import { SKIP_PRICE_LOOKUP_COINGECKO_ID } from '../common/constants/magics'
import WalletApiProxy from '../common/wallet_api_proxy'
import { BraveWallet, SupportedCoinTypes, SupportedTestNetworks } from '../constants/types'
import {
  externalWalletProviders
} from '../common/async/brave_rewards_api_proxy'

export const getPriceIdForToken = (
  token: Pick<
    BraveWallet.BlockchainToken,
    | 'contractAddress'
    | 'symbol'
    | 'coingeckoId'
    | 'chainId'
    >
) => {
  if (token?.coingeckoId) {
    return token.coingeckoId.toLowerCase()
  }

  // Skip price of testnet tokens other than goerli-eth
  if (SupportedTestNetworks.includes(token.chainId)) {
    // Goerli ETH has a real-world value
    if (
      token.chainId === BraveWallet.GOERLI_CHAIN_ID &&
      !token.contractAddress
    ) {
      return 'goerli-eth' // coingecko id
    }
    return SKIP_PRICE_LOOKUP_COINGECKO_ID
  }

  const isEthereumNetwork = token.chainId === BraveWallet.MAINNET_CHAIN_ID
  if (
    (isEthereumNetwork ||
      externalWalletProviders
        .includes(token.chainId)
    ) &&
    token.contractAddress
  ) {
    return token.contractAddress.toLowerCase()
  }

  return token.symbol.toLowerCase()
}

export function handleEndpointError(
  endpointName: string,
  friendlyMessage: string,
  error: any
) {
  const message = `${friendlyMessage}: ${error?.message || error}`
  console.log(`error in: ${endpointName || 'endpoint'}: ${message}`)
  console.error(error)
  return {
    error: friendlyMessage
  }
}

export async function getEnabledCoinTypes(api: WalletApiProxy) {
  const {
    isFilecoinEnabled, isSolanaEnabled, isBitcoinEnabled
  } = (await api.walletHandler.getWalletInfo()).walletInfo

  // Get All Networks
  return SupportedCoinTypes.filter((coin) => {
    // MULTICHAIN: While we are still in development for FIL and SOL,
    // we will not use their networks unless enabled by brave://flags
    return (
      (coin === BraveWallet.CoinType.FIL && isFilecoinEnabled) ||
      (coin === BraveWallet.CoinType.SOL && isSolanaEnabled) ||
      (coin === BraveWallet.CoinType.BTC && isBitcoinEnabled) ||
      coin === BraveWallet.CoinType.ETH
    )
  })
}

export async function getAllNetworksList(api: WalletApiProxy) {
  const { jsonRpcService } = api

  const enabledCoinTypes = await getEnabledCoinTypes(api)

  // Get All Networks
  const networks = (
    await mapLimit(
      enabledCoinTypes, 10, (async (coin: number) => {
        const { networks } = await jsonRpcService.getAllNetworks(coin)
        return networks
      })
    )
  ).flat(1)

  return networks
}

export async function getNetwork(
  api: WalletApiProxy,
  arg: Pick<BraveWallet.NetworkInfo, 'chainId' | 'coin'>
): Promise<BraveWallet.NetworkInfo | undefined> {
  const networksList = await getAllNetworksList(api)

  return networksList.find(
    (n) => n.chainId === arg.chainId && n.coin === arg.coin
  )
}

export async function getVisibleNetworksList(
  api: WalletApiProxy
) {
  const { jsonRpcService } = api

  const enabledCoinTypes = await getEnabledCoinTypes(api)

  const networks = (
    await mapLimit(enabledCoinTypes, 10, async (coin: number) => {
      const { networks } = await jsonRpcService.getAllNetworks(coin)
      const { chainIds: hiddenChainIds } =
        await jsonRpcService.getHiddenNetworks(coin)
      return networks.filter((n) => !hiddenChainIds.includes(n.chainId))
    })
  ).flat(1)

  return networks
}
