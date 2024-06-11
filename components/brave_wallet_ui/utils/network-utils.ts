// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { assertNotReached } from 'chrome://resources/js/assert.js'

// types
import { BraveWallet, SerializableSolanaTxData } from '../constants/types'

export const emptyNetwork: BraveWallet.NetworkInfo = {
  chainId: '',
  chainName: '',
  activeRpcEndpointIndex: 0,
  rpcEndpoints: [],
  blockExplorerUrls: [],
  iconUrls: [],
  symbol: '',
  symbolName: '',
  decimals: 0,
  coin: BraveWallet.CoinType.ETH,
  supportedKeyrings: []
}

export const getNetworkInfo = (
  chainId: string,
  coin: BraveWallet.CoinType,
  list: BraveWallet.NetworkInfo[]
) => {
  for (let it of list) {
    if (it.chainId === chainId && it.coin === coin) {
      return it
    }
  }
  return emptyNetwork
}

export const networkSupportsAccount = (
  network: Pick<BraveWallet.NetworkInfo, 'coin' | 'supportedKeyrings'>,
  accountId: BraveWallet.AccountId
) => {
  return (
    network.coin === accountId.coin &&
    network.supportedKeyrings.includes(accountId.keyringId)
  )
}

export const filterNetworksForAccount = (
  networks: BraveWallet.NetworkInfo[],
  accountId: BraveWallet.AccountId
): BraveWallet.NetworkInfo[] => {
  if (!networks) {
    return []
  }
  return networks.filter((network) =>
    networkSupportsAccount(network, accountId)
  )
}

export const getTokensNetwork = (
  networks: BraveWallet.NetworkInfo[],
  token: BraveWallet.BlockchainToken
): BraveWallet.NetworkInfo => {
  if (!networks) {
    return emptyNetwork
  }

  const network = networks.filter((n) => n.chainId === token.chainId)
  if (network.length > 1) {
    return (
      network?.find(
        (n) => n.symbol.toLowerCase() === token.symbol.toLowerCase()
      ) ?? emptyNetwork
    )
  }

  return network[0] ?? emptyNetwork
}

export type TxDataPresence = {
  ethTxData?: Partial<BraveWallet.TxDataUnion['ethTxData']> | undefined
  ethTxData1559?: Partial<BraveWallet.TxDataUnion['ethTxData1559']> | undefined
  solanaTxData?:
    | Partial<BraveWallet.TxDataUnion['solanaTxData']>
    | SerializableSolanaTxData
    | undefined
  filTxData?: Partial<BraveWallet.TxDataUnion['filTxData']> | undefined
  btcTxData?: Partial<BraveWallet.TxDataUnion['btcTxData']> | undefined
  zecTxData?: Partial<BraveWallet.TxDataUnion['zecTxData']> | undefined
}

export const getCoinFromTxDataUnion = <T extends TxDataPresence>(
  txDataUnion: T
): BraveWallet.CoinType => {
  if (txDataUnion.ethTxData || txDataUnion.ethTxData1559) {
    return BraveWallet.CoinType.ETH
  }
  if (txDataUnion.filTxData) {
    return BraveWallet.CoinType.FIL
  }
  if (txDataUnion.solanaTxData) {
    return BraveWallet.CoinType.SOL
  }
  if (txDataUnion.btcTxData) {
    return BraveWallet.CoinType.BTC
  }
  if (txDataUnion.zecTxData) {
    return BraveWallet.CoinType.ZEC
  }

  assertNotReached('Unknown transaction coin')
}

export const reduceNetworkDisplayName = (name?: string) => {
  if (!name) {
    return ''
  }
  return name.split(' ')[0]
}
