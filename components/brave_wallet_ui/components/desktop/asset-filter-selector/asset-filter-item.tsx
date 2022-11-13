// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { AssetFilterOption } from '../../../constants/types'

// Styled Components
import {
  NetworkItemButton,
  NetworkName,
  NetworkItemWrapper,
  BigCheckMark
} from '../network-filter-selector/style'

interface Props {
  selectedAssetFilterItem: AssetFilterOption
  assetFilterItem: AssetFilterOption
  onSelectAssetFilterItem: (assetFilterItem: AssetFilterOption) => void
}

export const AssetFilterItem = (props: Props) => {
  const {
    selectedAssetFilterItem,
    assetFilterItem,
    onSelectAssetFilterItem
  } = props

  const onClickSelectAssetFilterItem = () => {
    onSelectAssetFilterItem(assetFilterItem)
  }

  return (
    <NetworkItemWrapper>
      <NetworkItemButton onClick={onClickSelectAssetFilterItem}>
        <NetworkName>{assetFilterItem.name}</NetworkName>
        {assetFilterItem.id === selectedAssetFilterItem.id &&
          <BigCheckMark />
        }
      </NetworkItemButton>
    </NetworkItemWrapper>
  )
}
