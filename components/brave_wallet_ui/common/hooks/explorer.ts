// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { getLocale } from '../../../common/locale'
import { BraveWallet, BlockExplorerUrlTypes } from '../../constants/types'
import Amount from '../../utils/amount'

export default function useExplorer (network: BraveWallet.NetworkInfo) {
  return React.useCallback(
    (type: BlockExplorerUrlTypes, value?: string, id?: string) => () => {
      const explorerURL = network.blockExplorerUrls[0]
      if (!explorerURL || !value) {
        alert(getLocale('braveWalletTransactionExplorerMissing'))
        return
      }

      const explorerIndex = explorerURL.lastIndexOf('?')

      const url = type === 'contract'
        ? `${explorerURL}/${value}?a=${new Amount(id ?? '').format()}`
        : (network.chainId === BraveWallet.SOLANA_TESTNET || network.chainId === BraveWallet.SOLANA_DEVNET)
          ? `${explorerURL.substring(0, explorerIndex)}/${type}/${value}${explorerURL.substring(explorerIndex)}`
          : `${explorerURL}/${type}/${value}`

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
