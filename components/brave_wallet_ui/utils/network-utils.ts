// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// types
import {
  BraveWallet,
  CoinType,
  SerializableSolanaTxData,
  WalletAccountTypeName
} from '../constants/types'

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
  coin: CoinType.ETH,
  supportedKeyrings: [],
  isEip1559: true
}

export const getNetworkInfo = (chainId: string, coin: CoinType, list: BraveWallet.NetworkInfo[]) => {
  for (let it of list) {
    if (it.chainId === chainId && it.coin === coin) {
      return it
    }
  }
  return emptyNetwork
}

export const networkSupportsAccount = (
  network: BraveWallet.NetworkInfo,
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
  return networks.filter((network) => networkSupportsAccount(network, accountId))
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
  solanaTxData?: Partial<BraveWallet.TxDataUnion['solanaTxData']>
    | SerializableSolanaTxData
    | undefined
  filTxData?: Partial<BraveWallet.TxDataUnion['filTxData']> | undefined
}

export const getCoinFromTxDataUnion = <T extends TxDataPresence> (txDataUnion: T): CoinType => {
  if (txDataUnion.filTxData) { return CoinType.FIL }
  if (txDataUnion.solanaTxData) { return CoinType.SOL }
  // TODO(apaymyshev): bitcoin support
  return CoinType.ETH
}

const EIP1559_SUPPORTED_ACCOUNT_TYPE_NAMES = [
  'Primary',
  'Secondary',
  'Ledger',
  'Trezor'
]

/**
 * Check if the keyring associated with the given account AND the network
 * support the EIP-1559 fee market for paying gas fees.
 *
 * This method can also be used to determine if the given parameters support
 * EVM Type-2 transactions. The return value is always false for non-EVM
 * networks.
 *
 * @param {BraveWallet.AccountInfo} account
 * @param {BraveWallet.NetworkInfo} network
 * @returns {boolean} Returns a boolean result indicating EIP-1559 support.
 */
export const hasEIP1559Support = (
  accountType: WalletAccountTypeName,
  network: BraveWallet.NetworkInfo
) => {
  return (
    EIP1559_SUPPORTED_ACCOUNT_TYPE_NAMES.includes(accountType) &&
    network.isEip1559
  )
}

export const reduceNetworkDisplayName = (name?: string) => {
  if (!name) {
    return ''
  }
  return name.split(' ')[0]
}
