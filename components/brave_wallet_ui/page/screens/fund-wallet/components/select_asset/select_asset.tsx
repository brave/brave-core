// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { VariableSizeList as List } from 'react-window'
import AutoSizer from 'react-virtualized-auto-sizer'
import { DialogProps } from '@brave/leo/react/dialog'
import Icon from '@brave/leo/react/icon'

// types
import {
  BraveWallet,
  MeldCryptoCurrency,
  MeldFiatCurrency,
  SpotPriceRegistry
} from '../../../../../constants/types'

// utils
import { getLocale } from '../../../../../../common/locale'
import Amount from '../../../../../utils/amount'

// styles
import { Column, Row } from '../../../../../components/shared/style'
import {
  AssetImage,
  AssetName,
  AssetNetwork,
  AssetPrice,
  SearchInput,
  Loader,
  AutoSizerStyle
} from './select_asset.style'
import { ContainerButton, Dialog, DialogTitle } from '../shared/style'
import {
  getAssetName,
  getAssetSymbol,
  getTokenPriceFromRegistry,
  getAssetIdKey
} from '../../../../../utils/meld_utils'

interface SelectAssetProps extends DialogProps {
  assets: MeldCryptoCurrency[]
  isLoadingAssets: boolean
  isLoadingSpotPrices: boolean
  selectedAsset?: MeldCryptoCurrency
  selectedFiatCurrency?: MeldFiatCurrency
  spotPriceRegistry?: SpotPriceRegistry
  onSelectAsset: (asset: MeldCryptoCurrency) => void
}

interface AssetListItemProps {
  index: number
  style: React.CSSProperties
  setSize: (index: number, size: number) => void

  asset: MeldCryptoCurrency
  isLoadingPrices: boolean
  assetPrice?: BraveWallet.AssetPrice
  fiatCurrencyCode?: string
  onSelect: (currency: MeldCryptoCurrency) => void
}

const assetItemHeight = 64

export const AssetListItem = ({
  index,
  style,
  setSize,
  asset,
  isLoadingPrices,
  assetPrice,
  fiatCurrencyCode,
  onSelect
}: AssetListItemProps) => {
  const { symbolImageUrl, currencyCode, chainName } = asset

  const assetSymbol = getAssetSymbol(asset)
  const assetName = getAssetName(asset)
  const networkDescription =
    currencyCode !== ''
      ? getLocale('braveWalletPortfolioAssetNetworkDescription')
          .replace('$1', assetSymbol ?? '')
          .replace('$2', chainName ?? '')
      : chainName

  // methods
  const handleSetSize = React.useCallback(
    (ref: HTMLButtonElement | null) => {
      if (ref) {
        setSize(index, ref.getBoundingClientRect().height)
      }
    },
    [index, setSize]
  )

  const formattedPrice = assetPrice
    ? new Amount(assetPrice.price).formatAsFiat(fiatCurrencyCode ?? '')
    : ''

  return (
    <div style={style}>
      <ContainerButton
        onClick={() => onSelect(asset)}
        ref={handleSetSize}
      >
        <Row
          justifyContent='flex-start'
          gap='16px'
        >
          <AssetImage src={`chrome://image?${symbolImageUrl}`} />
          <Column alignItems='flex-start'>
            <AssetName>{assetName}</AssetName>
            <AssetNetwork>{networkDescription}</AssetNetwork>
          </Column>
        </Row>
        <Row
          justifyContent='flex-end'
          gap='16px'
        >
          {isLoadingPrices ? (
            <Loader />
          ) : (
            <AssetPrice>{formattedPrice}</AssetPrice>
          )}
        </Row>
      </ContainerButton>
    </div>
  )
}

export const SelectAsset = (props: SelectAssetProps) => {
  const {
    assets,
    selectedAsset,
    isLoadingAssets,
    isLoadingSpotPrices,
    selectedFiatCurrency,
    spotPriceRegistry,
    onSelectAsset,
    ...rest
  } = props

  // state
  const [searchText, setSearchText] = React.useState('')

  // refs
  const listRef = React.useRef<List | null>(null)
  const itemSizes = React.useRef<number[]>(
    new Array(assets.length).fill(assetItemHeight)
  )

  // memos
  const searchResults = React.useMemo(() => {
    if (searchText === '') return assets

    return assets.filter((asset) => {
      return (
        asset?.name?.toLowerCase().includes(searchText.toLowerCase()) ||
        asset.currencyCode.toLowerCase().includes(searchText.toLowerCase())
      )
    })
  }, [assets, searchText])

  // methods
  const getListItemKey = (index: number, assets: MeldCryptoCurrency[]) => {
    return getAssetIdKey(assets[index])
  }

  const getSize = React.useCallback((index: number) => {
    return itemSizes.current[index] || assetItemHeight
  }, [])

  const setSize = React.useCallback((index: number, size: number) => {
    // Performance: Only update the sizeMap and reset cache if an actual value
    // changed
    if (itemSizes.current[index] !== size && size > -1) {
      itemSizes.current[index] = size
      if (listRef.current) {
        // Clear cached data and rerender
        listRef.current.resetAfterIndex(0)
      }
    }
  }, [])

  return (
    <Dialog
      {...rest}
      showClose
      size='mobile'
    >
      <DialogTitle slot='title'>Select Asset</DialogTitle>
      <Row
        padding='24px 0 0 0'
        slot='subtitle'
      >
        <SearchInput
          placeholder='Search currency'
          onInput={(e) => setSearchText(e.value)}
        >
          <Icon
            name='search'
            slot='left-icon'
          />
        </SearchInput>
      </Row>
      <Column
        width='100%'
        height='80vh'
        justifyContent='flex-start'
      >
        {isLoadingAssets && (
          <Row justifyContent='center'>
            <Loader />
          </Row>
        )}
        {searchResults.length === 0 && !isLoadingAssets ? (
          <Row
            justifyContent='center'
            alignItems='center'
          >
            No available assets
          </Row>
        ) : (
          <AutoSizer style={AutoSizerStyle}>
            {function ({ width, height }: { width: number; height: number }) {
              return (
                <List
                  itemKey={getListItemKey}
                  width={width}
                  height={height}
                  itemCount={searchResults.length}
                  itemSize={getSize}
                  itemData={searchResults}
                  children={({ data, index, style }) => (
                    <AssetListItem
                      index={index}
                      style={style}
                      setSize={setSize}
                      asset={data[index]}
                      isLoadingPrices={isLoadingSpotPrices}
                      assetPrice={
                        spotPriceRegistry
                          ? getTokenPriceFromRegistry(
                              spotPriceRegistry,
                              data[index]
                            )
                          : undefined
                      }
                      fiatCurrencyCode={selectedFiatCurrency?.currencyCode}
                      onSelect={onSelectAsset}
                    />
                  )}
                />
              )
            }}
          </AutoSizer>
        )}
      </Column>
    </Dialog>
  )
}
