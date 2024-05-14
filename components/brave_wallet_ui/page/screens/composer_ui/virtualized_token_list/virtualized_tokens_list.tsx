// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { VariableSizeList as List } from 'react-window'
import AutoSizer from 'react-virtualized-auto-sizer'

// Utils
import { getAssetIdKey } from '../../../../utils/asset-utils'
import { getTokenPriceFromRegistry } from '../../../../utils/pricing-utils'

// Types
import { BraveWallet, SpotPriceRegistry } from '../../../../constants/types'

// Components
import {
  TokenListItem //
} from '../token_list_item/token_list_item'

// Styled Components
import {
  AutoSizerStyle,
  listItemInitialHeight
} from './virtualized_tokens_list.style'

interface VirtualizedTokensListProps {
  tokenList: BraveWallet.BlockchainToken[]
  selectedFromToken?: BraveWallet.BlockchainToken
  selectedToToken?: BraveWallet.BlockchainToken
  selectingFromOrTo: 'from' | 'to'
  spotPriceRegistry?: SpotPriceRegistry
  isLoadingSpotPrices?: boolean
  firstNoBalanceTokenKey?: string
  modalType: 'send' | 'swap' | 'bridge'
  userTokenBalances: Record<string, string>
  onViewTokenDetails: (token: BraveWallet.BlockchainToken) => void
  onSelectToken: (token: BraveWallet.BlockchainToken) => void
  getAllAccountsWithBalance: (
    token: BraveWallet.BlockchainToken
  ) => BraveWallet.AccountInfo[]
}

interface ListItemProps extends Omit<VirtualizedTokensListProps, 'tokenList'> {
  index: number
  data: BraveWallet.BlockchainToken[]
  style: React.CSSProperties
  setSize: (index: number, size: number) => void
}

const getListItemKey = (
  index: number,
  tokenList: BraveWallet.BlockchainToken[]
) => {
  const token = tokenList[index]
  return getAssetIdKey(token)
}

const ListItem = (props: ListItemProps) => {
  const {
    index,
    data,
    onSelectToken,
    style,
    selectedFromToken,
    selectedToToken,
    selectingFromOrTo,
    setSize,
    spotPriceRegistry,
    isLoadingSpotPrices,
    firstNoBalanceTokenKey,
    modalType,
    userTokenBalances,
    getAllAccountsWithBalance,
    onViewTokenDetails
  } = props
  const token = data[index]

  const disabledText = React.useMemo(() => {
    if (
      selectingFromOrTo === 'to' &&
      selectedFromToken?.contractAddress === token.contractAddress &&
      selectedFromToken?.coin === token.coin &&
      selectedFromToken?.chainId === token.chainId
    ) {
      return 'braveWalletFromToken'
    }
    return selectingFromOrTo === 'from' &&
      selectedToToken?.contractAddress === token.contractAddress &&
      selectedToToken?.coin === token.coin &&
      selectedToToken?.chainId === token.chainId
      ? 'braveWalletToToken'
      : undefined
  }, [selectedFromToken, selectedToToken, token, selectingFromOrTo])

  const groupingLabel = React.useMemo(() => {
    if (
      modalType === 'swap' &&
      index === 0 &&
      firstNoBalanceTokenKey !== getAssetIdKey(token)
    ) {
      return 'owned'
    }
    return modalType === 'swap' &&
      firstNoBalanceTokenKey === getAssetIdKey(token)
      ? 'not-owned'
      : undefined
  }, [modalType, index, token, firstNoBalanceTokenKey])

  const handleSetSize = React.useCallback(
    (ref: HTMLDivElement | null) => {
      if (ref) {
        setSize(index, ref.getBoundingClientRect().height)
      }
    },
    [index, setSize]
  )

  return (
    <div style={style}>
      <TokenListItem
        onClick={() => onSelectToken(token)}
        onViewTokenDetails={onViewTokenDetails}
        token={token}
        disabledText={disabledText}
        isLoadingSpotPrice={isLoadingSpotPrices}
        spotPrice={
          spotPriceRegistry
            ? getTokenPriceFromRegistry(spotPriceRegistry, token)
            : undefined
        }
        ref={handleSetSize}
        tokenHasMultipleAccounts={getAllAccountsWithBalance(token).length > 1}
        groupingLabel={groupingLabel}
        balance={userTokenBalances[getAssetIdKey(token)]}
      />
    </div>
  )
}

export const VirtualizedTokenList = (props: VirtualizedTokensListProps) => {
  const {
    tokenList,
    onSelectToken,
    selectedFromToken,
    selectedToToken,
    selectingFromOrTo,
    spotPriceRegistry,
    isLoadingSpotPrices,
    firstNoBalanceTokenKey,
    modalType,
    userTokenBalances,
    onViewTokenDetails,
    getAllAccountsWithBalance
  } = props

  // Refs
  const listRef = React.useRef<List | null>(null)
  const itemSizes = React.useRef<number[]>(
    new Array(tokenList.length).fill(listItemInitialHeight)
  )

  // Methods
  const onListMount = React.useCallback(
    (ref: List | null) => {
      if (ref) {
        // Set ref on mount
        listRef.current = ref
        // Clear cached data and rerender
        listRef.current.resetAfterIndex(0)
      }
    },
    [listRef]
  )

  const setSize = React.useCallback(
    (index: number, size: number) => {
      // Performance: Only update the sizeMap and reset cache if an actual value
      // changed
      if (itemSizes.current[index] !== size && size > -1) {
        itemSizes.current[index] = size
        if (listRef.current) {
          // Clear cached data and rerender
          listRef.current.resetAfterIndex(0)
        }
      }
    },
    [itemSizes, listRef]
  )

  const getSize = React.useCallback(
    (index: number) => {
      return itemSizes.current[index] || listItemInitialHeight
    },
    [itemSizes]
  )

  return (
    <AutoSizer style={AutoSizerStyle}>
      {function ({ height, width }: { height: number; width: number }) {
        return (
          <List
            ref={onListMount}
            width={width}
            height={height}
            itemData={tokenList}
            itemCount={tokenList.length}
            itemSize={getSize}
            itemKey={getListItemKey}
            overscanCount={10}
            children={(itemProps) => (
              <ListItem
                {...itemProps}
                selectedFromToken={selectedFromToken}
                selectedToToken={selectedToToken}
                selectingFromOrTo={selectingFromOrTo}
                onSelectToken={onSelectToken}
                setSize={setSize}
                spotPriceRegistry={spotPriceRegistry}
                isLoadingSpotPrices={isLoadingSpotPrices}
                getAllAccountsWithBalance={getAllAccountsWithBalance}
                firstNoBalanceTokenKey={firstNoBalanceTokenKey}
                onViewTokenDetails={onViewTokenDetails}
                modalType={modalType}
                userTokenBalances={userTokenBalances}
              />
            )}
          />
        )
      }}
    </AutoSizer>
  )
}
