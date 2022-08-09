// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { getLocale } from '../../../common/locale'
import { BraveWallet, BlockExplorerUrlTypes } from '../../constants/types'
import Amount from '../../utils/amount'

export function buildExplorerUrl (
  network: BraveWallet.NetworkInfo, type: BlockExplorerUrlTypes,
  value?: string, id?: string) {
  const explorerURL = network.blockExplorerUrls[0]

  const fallbackURL = `${explorerURL}/${value}`

  if (type === 'nft') {
    return id ? `${explorerURL}/token/${value}?a=${new Amount(id).format()}` : fallbackURL
  }

  if (type === 'contract') {
    return id ? `${explorerURL}/${value}?a=${new Amount(id).format()}` : fallbackURL
  }

  const isFileCoinNet =
    (network.chainId === BraveWallet.FILECOIN_TESTNET || network.chainId === BraveWallet.FILECOIN_MAINNET)

  const isSolanaDevOrTestNet =
    (network.chainId === BraveWallet.SOLANA_TESTNET || network.chainId === BraveWallet.SOLANA_DEVNET)

  if (isFileCoinNet) {
    return `${explorerURL}?cid=${value}`
  } else if (isSolanaDevOrTestNet) {
    const explorerIndex = explorerURL.lastIndexOf('?')
    return `${explorerURL.substring(0, explorerIndex)}/${type}/${value}${explorerURL.substring(explorerIndex)}`
  } else {
    return `${explorerURL}/${type}/${value}`
  }
}

export default function useExplorer (network: BraveWallet.NetworkInfo) {
  return React.useCallback(
    (type: BlockExplorerUrlTypes, value?: string, id?: string) => () => {
      const explorerBaseURL = network.blockExplorerUrls[0]
      if (!explorerBaseURL || !value) {
        alert(getLocale('braveWalletTransactionExplorerMissing'))
        return
      }

      const url = buildExplorerUrl(network, type, value, id)

      if (!chrome.tabs) {
        window.open(url, '_blank')
      } else {
        chrome.tabs.create({ url: url }, () => {
          if (chrome.runtime.lastError) {
            console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
          }
        })
      }
    },
    [network]
  )
}
