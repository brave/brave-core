// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import {
  getLocale
} from '../../../../../../../common/locale'
import Amount from '../../../../../../utils/amount'

// Queries
import {
  useGetSelectedChainQuery
} from '../../../../../../common/slices/api.slice'

// Types
import {
  BraveWallet
} from '../../../../../../constants/types'

// Components
import {
  SearchWithNetworkSelector
} from '../search-with-network-selector/search-with-network-selector'
import {
  StandardModal
} from '../../modals/standard-modal/standard-modal'
import {
  VirtualizedTokenList
} from '../virtualized-swap-token-list/virtualized-swap-tokens-list'

// Styled Components
import {
  Button,
  ScrollContainer
} from './select-token-modal.style'
import {
  Row,
  Text,
  VerticalDivider,
  IconButton,
  HiddenResponsiveRow,
  Icon
} from '../../shared-swap.styles'

interface Props {
  onClose: () => void
  onSelectToken: (token: BraveWallet.BlockchainToken) => void
  getCachedAssetBalance: (token: BraveWallet.BlockchainToken) => Amount
  disabledToken: BraveWallet.BlockchainToken | undefined
  selectingFromOrTo: 'from' | 'to'
  getNetworkAssetsList: (
    network: BraveWallet.NetworkInfo
  ) => BraveWallet.BlockchainToken[]
}

export const SelectTokenModal = React.forwardRef<HTMLDivElement, Props>(
  (props: Props, forwardedRef) => {
    const {
      onClose,
      onSelectToken,
      getCachedAssetBalance,
      getNetworkAssetsList,
      disabledToken,
      selectingFromOrTo
    } = props

    // Queries
    const { data: selectedNetwork } = useGetSelectedChainQuery()

    // State
    const [hideTokensWithZeroBalances, setHideTokensWithZeroBalances] =
      React.useState<boolean>(true)
    const [searchValue, setSearchValue] = React.useState<string>('')

    // Methods
    const toggleHideTokensWithZeroBalances = React.useCallback(() => {
      setHideTokensWithZeroBalances(prev => !prev)
    }, [])

    const handleOnSearchChanged = React.useCallback((value: string) => {
      setSearchValue(value)
    }, [])

    // Memos
    const networkAssetsList = React.useMemo(() => {
      if (selectedNetwork) {
        return getNetworkAssetsList(selectedNetwork)
      }
      return []
    }, [getNetworkAssetsList, selectedNetwork])

    const buttonText: string = React.useMemo(() => {
      return hideTokensWithZeroBalances
        ? getLocale('braveSwapShowTokensWithZeroBalances')
        : getLocale('braveSwapHideTokensWithZeroBalances')
    }, [getLocale, hideTokensWithZeroBalances])

    const filteredTokenListBySearch: BraveWallet.BlockchainToken[] =
      React.useMemo(() => {
        if (searchValue === '') {
          return networkAssetsList
        }
        return networkAssetsList.filter(
          (token: BraveWallet.BlockchainToken) =>
            token.name.toLowerCase().startsWith(searchValue.toLowerCase()) ||
            token.symbol.toLowerCase().startsWith(searchValue.toLowerCase())
        )
      }, [networkAssetsList, searchValue])

    const tokenListWithBalances: BraveWallet.BlockchainToken[] =
      React.useMemo(() => {
        return filteredTokenListBySearch
          .filter((token: BraveWallet.BlockchainToken) =>
            getCachedAssetBalance(token).gt(0)
          )
      }, [filteredTokenListBySearch, getCachedAssetBalance])

    const filteredTokenList: BraveWallet.BlockchainToken[] =
      React.useMemo(() => {
        if (tokenListWithBalances.length === 0) {
          return filteredTokenListBySearch
        }
        if (hideTokensWithZeroBalances) {
          return tokenListWithBalances
        }
        return filteredTokenListBySearch
      }, [
        filteredTokenListBySearch,
        hideTokensWithZeroBalances,
        tokenListWithBalances
      ])

    const showZeroBalanceButton: boolean = React.useMemo(() => {
      return tokenListWithBalances.length !== 0
    }, [tokenListWithBalances])

    // Effects
    React.useEffect(() => {
      if (selectingFromOrTo === 'to') {
        setHideTokensWithZeroBalances(false)
      }
    }, [selectingFromOrTo])

    // render
    return (
      <StandardModal
        ref={forwardedRef}
        modalHeight={
          hideTokensWithZeroBalances
            ? 'standard'
            : 'full'
        }
      >
        <Row rowWidth='full' horizontalPadding={24} verticalPadding={20}>
          <Text textSize='18px' responsiveTextSize='20px' isBold={true}>
            {getLocale('braveSwapSelectAToken')}
          </Text>
          <IconButton onClick={onClose}>
            <Icon name='close' size={24} />
          </IconButton>
        </Row>
        <Row rowWidth='full' horizontalPadding={20} marginBottom={16}>
          <SearchWithNetworkSelector
            onSearchChanged={handleOnSearchChanged}
            searchValue={searchValue}
            networkSelectorDisabled={selectingFromOrTo === 'to'}
          />
        </Row>
        <HiddenResponsiveRow maxWidth={570}>
          <VerticalDivider />
        </HiddenResponsiveRow>
        <ScrollContainer
          columnWidth='full'
          verticalAlign='flex-start'
          verticalPadding={8}
        >
          {filteredTokenList.length !== 0 && (
            <VirtualizedTokenList
              disabledToken={disabledToken}
              getCachedAssetBalance={getCachedAssetBalance}
              onSelectToken={onSelectToken}
              tokenList={filteredTokenList}
            />
          )}
        </ScrollContainer>
        {showZeroBalanceButton && (
          <Button
            onClick={toggleHideTokensWithZeroBalances}
            buttonStyle='square'
            buttonWidth='full'
            horizontalMargin={0}
            verticalMargin={0}
          >
            {buttonText}
          </Button>
        )}
      </StandardModal>
    )
  }
)
