// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '../../../../../common/locale'

// types
import {
  BlockExplorerUrlTypes, //
  BraveWallet
} from '../../../../constants/types'

// hooks
import { useExplorer } from '../../../../common/hooks/explorer'

// styles
import { InlineAddressButton, LaunchIcon } from '../style'

export type ChainInfo = Pick<
  BraveWallet.NetworkInfo,
  'symbol' | 'chainId' | 'iconUrls' | 'chainName' | 'blockExplorerUrls' | 'coin'
>

export function InlineViewOnBlockExplorerIconButton({
  address,
  id,
  network,
  urlType
}: {
  address: string
  id?: string
  network: ChainInfo
  urlType: BlockExplorerUrlTypes
}) {
  // custom hooks
  const onClickViewOnBlockExplorer = useExplorer(network)

  return (
    <InlineAddressButton
      title={getLocale('braveWalletTransactionExplorer')}
      onClick={onClickViewOnBlockExplorer(urlType, address, id)}
    >
      <LaunchIcon />
    </InlineAddressButton>
  )
}
