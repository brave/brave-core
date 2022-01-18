// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { getLocale } from '../../../common/locale'
import { BraveWallet, BlockExplorerUrlTypes } from '../../constants/types'
import { hexToNumber } from '../../utils/format-balances'

export default function useExplorer (network: BraveWallet.EthereumChain) {
  return React.useCallback(
    (type: BlockExplorerUrlTypes, value?: string, id?: string) => () => {
      const explorerURL = network.blockExplorerUrls[0]
      if (!explorerURL || !value) {
        alert(getLocale('braveWalletTransactionExplorerMissing'))
        return
      }

      const url = type === 'contract' ? `${explorerURL}/${value}?a=${hexToNumber(id ?? '', true)}` : `${explorerURL}/${type}/${value}`

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
