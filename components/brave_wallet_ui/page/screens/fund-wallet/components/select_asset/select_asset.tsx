// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { VariableSizeList as List } from 'react-window'
import AutoSizer from 'react-virtualized-auto-sizer'
import { DialogProps } from '@brave/leo/react/dialog'

// Selectors
import {
  useSafeUISelector //
} from '../../../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../../../common/selectors'

// Options
import {
  AllNetworksOption //
} from '../../../../../options/network-filter-options'

// Queries
import {
  useGetAllKnownNetworksQuery //
} from '../../../../../common/slices/api.slice'

// Types
import {
  BraveWallet,
  MeldCryptoCurrency,
  MeldFiatCurrency,
  SpotPriceRegistry
} from '../../../../../constants/types'

// Utils
import {
  getAssetSymbol,
  getTokenPriceFromRegistry,
  getAssetIdKey,
  getMeldTokensCoinType
} from '../../../../../utils/meld_utils'
import { getLocale } from '../../../../../../common/locale'
import Amount from '../../../../../utils/amount'

// Components
import {
  CreateNetworkIcon //
} from '../../../../../components/shared/create-network-icon'
import {
  NetworkFilterSelector //
} from '../../../../../components/desktop/network-filter-selector'
import {
  SearchBar //
} from '../../../../../components/shared/search-bar/index'
import {
  BottomSheet //
} from '../../../../../components/shared/bottom_sheet/bottom_sheet'

// Styled Components
import { Column, Row } from '../../../../../components/shared/style'
import {
  AssetImage,
  AssetName,
  AssetNetwork,
  AssetPrice,
  Loader,
  AutoSizerStyle,
  SearchAndNetworkFilterRow
} from './select_asset.style'
import {
  ContainerButton,
  Dialog,
  DialogTitle,
  ListTitle,
  IconsWrapper,
  NetworkIconWrapper
} from '../shared/style'

interface SelectAssetProps extends DialogProps {
  assets: MeldCryptoCurrency[]
  isLoadingAssets: boolean
  isLoadingSpotPrices: boolean
  selectedAsset?: MeldCryptoCurrency
  selectedFiatCurrency?: MeldFiatCurrency
  spotPriceRegistry?: SpotPriceRegistry
  isOpen: boolean
  onClose: () => void
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
  network?: BraveWallet.NetworkInfo
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
  onSelect,
  network
}: AssetListItemProps) => {
  const { symbolImageUrl, currencyCode, chainName } = asset

  // Computed
  const assetSymbol = getAssetSymbol(asset)
  const networkDescription =
    currencyCode !== ''
      ? getLocale('braveWalletPortfolioAssetNetworkDescription')
          .replace('$1', assetSymbol ?? '')
          .replace('$2', chainName ?? '')
      : chainName
  const formattedPrice = assetPrice
    ? new Amount(assetPrice.price).formatAsFiat(fiatCurrencyCode ?? '', 4)
    : ''

  // Methods
  const handleSetSize = React.useCallback(
    (ref: HTMLButtonElement | null) => {
      if (ref) {
        setSize(index, ref.getBoundingClientRect().height)
      }
    },
    [index, setSize]
  )

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
          <IconsWrapper>
            <AssetImage src={`chrome://image?${symbolImageUrl}`} />
            <NetworkIconWrapper>
              <CreateNetworkIcon
                network={network}
                marginRight={0}
                size='tiny'
              />
            </NetworkIconWrapper>
          </IconsWrapper>
          <Column alignItems='flex-start'>
            <AssetName>{asset.name}</AssetName>
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
    isOpen,
    onClose,
    ...rest
  } = props

  // Selectors
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // State
  const [searchText, setSearchText] = React.useState('')
  const [selectedNetworkFilter, setSelectedNetworkFilter] =
    React.useState<BraveWallet.NetworkInfo>(AllNetworksOption)

  // Refs
  const listRef = React.useRef<List | null>(null)
  const itemSizes = React.useRef<number[]>(
    new Array(assets.length).fill(assetItemHeight)
  )

  // Queries
  const { data: networkList = [] } = useGetAllKnownNetworksQuery()

  // Memos
  const assetsFilteredByNetwork = React.useMemo(() => {
    if (selectedNetworkFilter.chainId === AllNetworksOption.chainId) {
      return assets
    }
    return assets.filter(
      (asset) => asset.chainId === selectedNetworkFilter.chainId
    )
  }, [selectedNetworkFilter, assets])

  const searchResults = React.useMemo(() => {
    if (searchText === '') return assetsFilteredByNetwork

    return assetsFilteredByNetwork.filter((asset) => {
      const assetSymbol = getAssetSymbol(asset).toLowerCase()
      const assetName = asset.name?.toLowerCase() ?? ''
      return (
        assetName.startsWith(searchText.toLowerCase()) ||
        assetSymbol.startsWith(searchText.toLowerCase()) ||
        asset?.contractAddress
          ?.toLowerCase()
          .startsWith(searchText.toLowerCase()) ||
        assetName.includes(searchText.toLowerCase()) ||
        assetSymbol.includes(searchText.toLowerCase())
      )
    })
  }, [assetsFilteredByNetwork, searchText])

  const networks = React.useMemo(() => {
    const allChainIds = assets.map((asset) => asset.chainId)
    let reducedChainIds = [...new Set(allChainIds)]
    return networkList.filter((network) =>
      reducedChainIds.includes(network.chainId)
    )
  }, [assets, networkList])

  // Methods
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

  const getAssetsNetwork = React.useCallback(
    (asset: MeldCryptoCurrency) => {
      return networkList.find(
        (network) =>
          network.chainId.toLowerCase() === asset.chainId?.toLowerCase() &&
          getMeldTokensCoinType(asset) === network.coin
      )
    },
    [networkList]
  )

  const updateSearchValue = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setSearchText(event.target.value)
    },
    []
  )

  const onSelectNetwork = React.useCallback(
    (network: BraveWallet.NetworkInfo) => {
      setSelectedNetworkFilter(network)
    },
    []
  )

  // Memos
  const selectAssetContent = React.useMemo(() => {
    return (
      <>
        <DialogTitle slot='title'>
          {getLocale('braveWalletSelectAsset')}
        </DialogTitle>
        <SearchAndNetworkFilterRow margin='24px 0 8px 0'>
          <SearchBar
            placeholder={getLocale('braveWalletSearchTokens')}
            action={updateSearchValue}
            autoFocus={true}
            value={searchText}
            isV2={true}
          />
          <NetworkFilterSelector
            networkListSubset={networks}
            onSelectNetwork={onSelectNetwork}
            selectedNetwork={selectedNetworkFilter}
            isV2={true}
            dropdownPosition='right'
          />
        </SearchAndNetworkFilterRow>
        <Column
          width='100%'
          height='80vh'
          justifyContent='flex-start'
        >
          {isLoadingAssets && (
            <Column height='100%'>
              <Loader />
            </Column>
          )}
          {searchResults.length === 0 && !isLoadingAssets ? (
            <Column height='100%'>
              {getLocale('braveWalletNoAvailableAssets')}
            </Column>
          ) : (
            <>
              <Row
                justifyContent='space-between'
                padding='16px'
              >
                <ListTitle>{getLocale('braveWalletAsset')}</ListTitle>
                <ListTitle>~ {getLocale('braveWalletPrice')}</ListTitle>
              </Row>
              <AutoSizer style={AutoSizerStyle}>
                {function ({
                  width,
                  height
                }: {
                  width: number
                  height: number
                }) {
                  return (
                    <List
                      itemKey={getListItemKey}
                      width={width}
                      height={height}
                      itemCount={searchResults.length}
                      itemSize={getSize}
                      itemData={searchResults}
                      style={{ scrollbarWidth: 'none' }}
                      children={({ data, index, style }) => (
                        <AssetListItem
                          index={index}
                          style={style}
                          setSize={setSize}
                          asset={data[index]}
                          network={getAssetsNetwork(data[index])}
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
            </>
          )}
        </Column>
      </>
    )
  }, [
    getAssetsNetwork,
    getSize,
    isLoadingAssets,
    isLoadingSpotPrices,
    networks,
    onSelectAsset,
    onSelectNetwork,
    searchResults,
    searchText,
    selectedFiatCurrency,
    selectedNetworkFilter,
    setSize,
    spotPriceRegistry,
    updateSearchValue
  ])

  if (isPanel) {
    return (
      <BottomSheet
        onClose={onClose}
        isOpen={isOpen}
      >
        <Column
          fullWidth={true}
          padding='0px 16px'
          height='90vh'
        >
          {selectAssetContent}
        </Column>
      </BottomSheet>
    )
  }

  return (
    <Dialog
      {...rest}
      isOpen={isOpen}
      onClose={onClose}
      showClose
    >
      {selectAssetContent}
    </Dialog>
  )
}
