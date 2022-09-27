// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  useSelector,
  useDispatch
} from 'react-redux'
import { WalletActions } from '../../../common/actions'

// Types
import { WalletState, AssetFilterOption } from '../../../constants/types'
import { AssetFilterOptions } from '../../../options/asset-filter-options'

// Components
import { AssetFilterItem } from './asset-filter-item'

// Styled Components
import {
  StyledWrapper,
  DropDown,
  DropDownButton,
  DropDownIcon,
  SelectorLeftSide,
  ClickAwayArea
} from '../network-filter-selector/style'

export const AssetFilterSelector = () => {
  const [showAssetFilter, setShowAssetFilter] = React.useState<boolean>(false)

  const selectedAssetFilter = useSelector(({ wallet }: { wallet: WalletState }) => wallet.selectedAssetFilter)

  const dispatch = useDispatch()

  const toggleShowAssetFilter = () => {
    setShowAssetFilter(!showAssetFilter)
  }

  const onSelectAndClose = (assetFilterItem: AssetFilterOption) => {
    dispatch(WalletActions.setSelectedAssetFilterItem(assetFilterItem))
    toggleShowAssetFilter()
  }

  const hideAssetFilter = () => {
    setShowAssetFilter(false)
  }

  return (
    <StyledWrapper>

      <DropDownButton
        onClick={toggleShowAssetFilter}>
        <SelectorLeftSide>
          {selectedAssetFilter.name}
        </SelectorLeftSide>
        <DropDownIcon />
      </DropDownButton>

      {showAssetFilter &&
        <DropDown>
          {AssetFilterOptions.map((assetFilterItem: AssetFilterOption) =>
            <AssetFilterItem
              key={assetFilterItem.id}
              assetFilterItem={assetFilterItem}
              onSelectAssetFilterItem={onSelectAndClose}
              selectedAssetFilterItem={selectedAssetFilter}
            />
          )}
        </DropDown>
      }
      {showAssetFilter &&
        <ClickAwayArea onClick={hideAssetFilter} />
      }
    </StyledWrapper >
  )
}
