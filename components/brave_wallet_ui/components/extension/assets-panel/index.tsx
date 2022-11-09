// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  BraveWallet,
  WalletRoutes,
  WalletAccountType
} from '../../../constants/types'

// utils
import { getLocale } from '../../../../common/locale'
import { getBalance } from '../../../utils/balance-utils'

// Styled Components
import {
  StyledWrapper,
  AddAssetButton
} from './style'

import { PortfolioAssetItem } from '../../desktop'
import { getAssetIdKey } from '../../../utils/asset-utils'
export interface Props {
  userAssetList: BraveWallet.BlockchainToken[]
  selectedAccount?: WalletAccountType
  onAddAsset: () => void
}

const AssetsPanel = (props: Props) => {
  const {
    userAssetList,
    selectedAccount,
    onAddAsset
  } = props

  const onClickAsset = (symbol: string) => () => {
    const url = `brave://wallet${WalletRoutes.Portfolio}/${symbol}`
    chrome.tabs.create({ url: url }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }

  return (
    <StyledWrapper>
      <AddAssetButton
        onClick={onAddAsset}
      >
        {getLocale('braveWalletAddAsset')}
      </AddAssetButton>
      {userAssetList?.map((asset) =>
        <PortfolioAssetItem
          action={onClickAsset(asset.symbol)}
          key={getAssetIdKey(asset)}
          assetBalance={getBalance(selectedAccount, asset)}
          token={asset}
          isPanel={true}
        />
      )}
    </StyledWrapper>
  )
}

export default AssetsPanel
