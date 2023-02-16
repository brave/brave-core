// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// types
import {
  BraveWallet,
  SerializableSolanaTxData
} from '../constants/types'

// utils
import { isFilecoinTestnetTx, isFilecoinTransaction } from './tx-utils'

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
  isEip1559: true
}

export const getNetworkInfo = (chainId: string, coin: BraveWallet.CoinType, list: BraveWallet.NetworkInfo[]) => {
  for (let it of list) {
    if (it.chainId === chainId && it.coin === coin) {
      return it
    }
  }
  return emptyNetwork
}

export const getNetworksByCoinType = (networks: BraveWallet.NetworkInfo[], coin: BraveWallet.CoinType): BraveWallet.NetworkInfo[] => {
  if (!networks) {
    return []
  }
  return networks.filter((network) => network.coin === coin)
}

export const getTokensNetwork = (networks: BraveWallet.NetworkInfo[], token: BraveWallet.BlockchainToken): BraveWallet.NetworkInfo => {
  if (!networks) {
    return emptyNetwork
  }

  const network = networks.filter((n) => n.chainId === token.chainId)
  if (network.length > 1) {
    return network?.find((n) => n.symbol.toLowerCase() === token.symbol.toLowerCase()) ?? emptyNetwork
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
}

export const getCoinFromTxDataUnion = <T extends TxDataPresence> (txDataUnion: T): BraveWallet.CoinType => {
  if (txDataUnion.filTxData) { return BraveWallet.CoinType.FIL }
  if (txDataUnion.solanaTxData) { return BraveWallet.CoinType.SOL }
  return BraveWallet.CoinType.ETH
}

export const getNetworkFromTXDataUnion = <
  T extends TxDataPresence,
  N extends BraveWallet.NetworkInfo
> (
  txDataUnion: T,
  networks: N[],
  selectedNetwork?: N | undefined
): N | undefined => {
  const tx = { txDataUnion }
  const coin = getCoinFromTxDataUnion(tx.txDataUnion)

  const isFilTx = isFilecoinTransaction(tx)
  const isFilTestNetTx = isFilecoinTestnetTx(tx)

  return (
    networks.find((network) => {
      switch (network.coin) {
        case BraveWallet.CoinType.ETH:
          return network.chainId === txDataUnion.ethTxData1559?.chainId

        case BraveWallet.CoinType.FIL:
          // fil addresses on testnet start with 't'
          return isFilTestNetTx
            ? network.chainId === BraveWallet.FILECOIN_TESTNET
            : isFilTx
            ? network.chainId === BraveWallet.FILECOIN_MAINNET
            : network.coin === coin

        // TODO: find a way to get SOL chainIds
        case BraveWallet.CoinType.SOL:
        default:
          return network.coin === coin
      }
    }) ?? selectedNetwork
  )
}

export function getFilecoinKeyringIdFromNetwork (
  network: Pick<BraveWallet.NetworkInfo, 'chainId' | 'coin'>
) {
  if (network.coin !== BraveWallet.CoinType.FIL) {
    return undefined
  }
  if (network.chainId === BraveWallet.FILECOIN_MAINNET) {
    return BraveWallet.FILECOIN_KEYRING_ID
  } else {
    return BraveWallet.FILECOIN_TESTNET_KEYRING_ID
  }
}
