// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  useSelector,
  useDispatch
} from 'react-redux'
import { WalletActions } from '../../../common/actions'

// Types
import { WalletState, AssetFilterOption } from '../../../constants/types'
import { AssetFilterOptions, HighToLowAssetsFilterOption } from '../../../options/asset-filter-options'
import { LOCAL_STORAGE_KEYS } from '../../../common/constants/local-storage-keys'

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
    window.localStorage.setItem(LOCAL_STORAGE_KEYS.PORTFOLIO_ASSET_FILTER_OPTION, assetFilterItem.id)
    dispatch(WalletActions.setSelectedAssetFilterItem(assetFilterItem.id))
    toggleShowAssetFilter()
  }

  const hideAssetFilter = () => {
    setShowAssetFilter(false)
  }

  // memos
  const selectedAssetFilterInfo = React.useMemo(() => {
    return AssetFilterOptions.find(item => item.id === selectedAssetFilter) ?? HighToLowAssetsFilterOption
  }, [selectedAssetFilter])

  return (
    <StyledWrapper>

      <DropDownButton
        onClick={toggleShowAssetFilter}>
        <SelectorLeftSide>
          {selectedAssetFilterInfo.name}
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
              selectedAssetFilterItem={selectedAssetFilterInfo}
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
