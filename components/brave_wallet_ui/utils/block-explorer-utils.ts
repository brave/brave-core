// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// types
import { BlockExplorerUrlTypes, BraveWallet } from '../constants/types'

// utils
import { getLocale } from '../../common/locale'
import Amount from './amount'

export const buildExplorerUrl = (
  network: Pick<BraveWallet.NetworkInfo, 'chainId' | 'blockExplorerUrls'>,
  type: BlockExplorerUrlTypes,
  value?: string,
  id?: string
) => {
  const explorerURL = network.blockExplorerUrls[0]

  const fallbackURL = `${explorerURL}/${value}`

  if (type === 'lifi') {
    return `https://scan.li.fi/tx/${value}`
  }

  if (type === 'nft') {
    return id
      ? `${explorerURL}/token/${value}?a=${new Amount(id).format()}`
      : fallbackURL
  }

  if (type === 'contract') {
    return `${explorerURL}/address/${value}/#code`
  }

  const isFileCoinNet =
    network.chainId === BraveWallet.FILECOIN_TESTNET ||
    network.chainId === BraveWallet.FILECOIN_MAINNET

  const isFileCoinEvmNet =
    network.chainId === BraveWallet.FILECOIN_ETHEREUM_MAINNET_CHAIN_ID ||
    network.chainId === BraveWallet.FILECOIN_ETHEREUM_TESTNET_CHAIN_ID

  const isSolanaMainNet = network.chainId === BraveWallet.SOLANA_MAINNET

  const isSolanaDevOrTestNet =
    network.chainId === BraveWallet.SOLANA_TESTNET ||
    network.chainId === BraveWallet.SOLANA_DEVNET

  const isZecNet = network.chainId === BraveWallet.Z_CASH_MAINNET ||
                   network.chainId === BraveWallet.Z_CASH_TESTNET

  if (isFileCoinNet) {
    return `${explorerURL}?cid=${value}`
  }

  if (isFileCoinEvmNet) {
    return `${explorerURL}/${value}`
  }

  if (isZecNet) {
    return `${explorerURL}/${value}`
  }

  if (isSolanaMainNet && type === 'token') {
    return `${explorerURL}/address/${value}`
  }

  if (isSolanaDevOrTestNet) {
    const explorerIndex = explorerURL.lastIndexOf('?')
    return `${explorerURL.substring(
      0,
      explorerIndex
    )}/${type}/${value}${explorerURL.substring(explorerIndex)}`
  }

  if (network.chainId === BraveWallet.BITCOIN_MAINNET) {
    if (type === 'tx') {
      return `${explorerURL}/transactions/btc/${value}`
    }
    if (type === 'address') {
      return `${explorerURL}/addresses/btc/${value}`
    }
    return `${explorerURL}/search?search=${value}`
  }

  return `${explorerURL}/${type}/${value}`
}

export const openBlockExplorerURL = ({
  id,
  network,
  type,
  value
}: {
  id?: string | undefined
  network?: Pick<
    BraveWallet.NetworkInfo,
    'chainId' | 'blockExplorerUrls'
  > | null
  type: BlockExplorerUrlTypes
  value?: string | undefined
}): (() => void) => {
  return () => {
    if (!network) {
      return
    }

    const explorerBaseURL = network.blockExplorerUrls[0]
    if (!explorerBaseURL || !value) {
      alert(getLocale('braveWalletTransactionExplorerMissing'))
      return
    }

    const url = buildExplorerUrl(network, type, value, id)

    if (!chrome.tabs) {
      window.open(url, '_blank', 'noreferrer')
    } else {
      chrome.tabs.create({ url: url }, () => {
        if (chrome.runtime.lastError) {
          console.error(
            'tabs.create failed: ' + chrome.runtime.lastError.message
          )
        }
      })
    }
  }
}
