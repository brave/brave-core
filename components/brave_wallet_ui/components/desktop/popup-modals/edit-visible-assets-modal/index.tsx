// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  BraveWallet,
  WalletState
} from '../../../../constants/types'

// options
import { AllNetworksOption } from '../../../../options/network-filter-options'

// utils
import { getLocale } from '../../../../../common/locale'
import { checkIfTokensMatch } from '../../../../utils/asset-utils'

// components
import {
  PopupModal
} from '../..'
import { NavButton } from '../../../extension'
import NetworkFilterWithSearch from '../../network-filter-with-search'
import { VirtualizedVisibleAssetsList } from './virtualized-visible-assets-list'
import { AddAsset } from '../../add-asset/add-asset'

// Styled Components
import {
  LoadIcon,
  LoadingWrapper,
  NoAssetButton,
  NoAssetRow,
  NoAssetText,
  StyledWrapper,
  TopRow
} from './style'

// hooks
import { useAssetManagement } from '../../../../common/hooks'
import { useSelector } from 'react-redux'
import {
  useGetSelectedChainQuery,
  useGetTokensRegistryQuery
} from '../../../../common/slices/api.slice'
import {
  getEntitiesListFromEntityState
} from '../../../../utils/entities.utils'
import {
  blockchainTokenEntityAdaptorInitialState
} from '../../../../common/slices/entities/blockchain-token.entity'


export interface Props {
  onClose: () => void
}

const EditVisibleAssetsModal = ({ onClose }: Props) => {
  // redux
  const userVisibleTokensInfo = useSelector(({ wallet }: { wallet: WalletState }) => wallet.userVisibleTokensInfo)

  // queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()
  const {
    data: tokenEntityState =
    blockchainTokenEntityAdaptorInitialState,
    isLoading
  } = useGetTokensRegistryQuery()

  // custom hooks
  const {
    onUpdateVisibleAssets
  } = useAssetManagement()

  // Token List States
  const [searchValue, setSearchValue] = React.useState<string>('')
  const [updatedTokensList, setUpdatedTokensList] = React.useState<BraveWallet.BlockchainToken[]>([])
  const [removedTokensList, setRemovedTokensList] = React.useState<BraveWallet.BlockchainToken[]>([])
  const [tokenContractAddress, setTokenContractAddress] = React.useState<string>('')
  const [selectedNetworkFilter, setSelectedNetworkFilter] = React.useState<BraveWallet.NetworkInfo>(AllNetworksOption)
  const [showNetworkDropDown, setShowNetworkDropDown] = React.useState<boolean>(false)

  // Modal UI States
  const [showAddCustomToken, setShowAddCustomToken] = React.useState<boolean>(false)

  // If a user removes all of their assets from the userVisibleTokenInfo list,
  // there is a check in the async/lib.ts folder that will still return the networks
  // native asset (example 'ETH') with the value of visible: false to prevent breaking
  // other parts of the wallet.

  // This method here is used to determine if that is the case
  // and allows us to handle our true visible lists.
  const isUserVisibleTokensInfoEmpty = React.useMemo((): boolean => {
    return userVisibleTokensInfo.length === 1 &&
      userVisibleTokensInfo[0].contractAddress === '' &&
      !userVisibleTokensInfo[0].visible
  }, [userVisibleTokensInfo])

  React.useEffect(() => {
    if (isUserVisibleTokensInfoEmpty) {
      return
    }
    setUpdatedTokensList(userVisibleTokensInfo)
  }, [userVisibleTokensInfo])

  // Memos
  const nativeAsset = React.useMemo(() => {
    return selectedNetwork && {
      contractAddress: '',
      decimals: selectedNetwork.decimals,
      isErc20: false,
      isErc721: false,
      isErc1155: false,
      isNft: false,
      isSpam: false,
      logo: selectedNetwork.iconUrls[0] ?? '',
      name: selectedNetwork.symbolName,
      symbol: selectedNetwork.symbol,
      visible: true,
      tokenId: '',
      coingeckoId: '',
      chainId: selectedNetwork.chainId,
      coin: selectedNetwork.coin
    }
  }, [selectedNetwork])

  const fullTokenListAllChains = React.useMemo(() => {
    return getEntitiesListFromEntityState(
      tokenEntityState,
      tokenEntityState.ids
    )
  }, [tokenEntityState])

  // Token list based on selectedNetworkFilter
  const tokenListForSelectedNetworks: BraveWallet.BlockchainToken[] =
    React.useMemo(() => {
      if (
        selectedNetworkFilter.chainId ===
        AllNetworksOption.chainId
      ) {
        return fullTokenListAllChains
      }

      const tokenIds =
        tokenEntityState
          .idsByChainId[selectedNetworkFilter.chainId]
      return getEntitiesListFromEntityState(tokenEntityState, tokenIds)
    }, [
      selectedNetworkFilter.chainId,
      tokenEntityState,
      fullTokenListAllChains
    ])

  // User tokens sorted by visibility
  const usersTokensSortedByVisibility
    : BraveWallet.BlockchainToken[] =
    React.useMemo(() => {
      return [...userVisibleTokensInfo]
        .sort((a, b) =>
          Number(b.visible) - Number(a.visible)
        )
    }, [userVisibleTokensInfo])

  // Users visible tokens based on selectedNetworkFilter
  const userVisibleTokensBySelectedNetwork
    : BraveWallet.BlockchainToken[] =
    React.useMemo(() => {
      if (
        selectedNetworkFilter.chainId ===
        AllNetworksOption.chainId
      ) {
        return usersTokensSortedByVisibility
      }
      return usersTokensSortedByVisibility
        .filter(
          (token) =>
            token.chainId ===
            selectedNetworkFilter.chainId
        )
    }, [
      usersTokensSortedByVisibility,
      selectedNetworkFilter.chainId
    ])

  // Constructed list based on Users Visible Tokens and Full Token List
  const tokenList: BraveWallet.BlockchainToken[] = React.useMemo(() => {
    const userVisibleContracts = isUserVisibleTokensInfoEmpty
      ? []
      : userVisibleTokensBySelectedNetwork.map((token) => token.contractAddress.toLowerCase())

    const fullAssetsListPlusNativeToken = userVisibleContracts.includes('') || !nativeAsset
      ? tokenListForSelectedNetworks
      : [nativeAsset, ...tokenListForSelectedNetworks]

    const filteredTokenRegistry = fullAssetsListPlusNativeToken
      .filter((token) => !userVisibleContracts.includes(token?.contractAddress?.toLowerCase()))

    return isUserVisibleTokensInfoEmpty
      ? filteredTokenRegistry
      : [...userVisibleTokensBySelectedNetwork, ...filteredTokenRegistry]
  }, [
    isUserVisibleTokensInfoEmpty,
    tokenListForSelectedNetworks,
    userVisibleTokensInfo,
    nativeAsset,
    userVisibleTokensBySelectedNetwork
  ])

  // Filtered token list based on user removed tokens
  const filteredOutRemovedTokens = React.useMemo(() => {
    return tokenList.filter((token) =>
      !removedTokensList.some((t) =>
        t.contractAddress.toLowerCase() === token.contractAddress.toLowerCase() &&
        t.tokenId === token.tokenId))
  }, [tokenList, removedTokensList])

  // Filtered token list based on search value
  const filteredTokenList = React.useMemo(() => {
    if (searchValue === '') {
      return filteredOutRemovedTokens
    }
    return filteredOutRemovedTokens.filter((item) => {
      return (
        item.name.toLowerCase() === searchValue.toLowerCase() ||
        item.name.toLowerCase().startsWith(searchValue.toLowerCase()) ||
        item.symbol.toLocaleLowerCase() === searchValue.toLowerCase() ||
        item.symbol.toLowerCase().startsWith(searchValue.toLowerCase()) ||
        item.contractAddress.toLocaleLowerCase() === searchValue.toLowerCase()
      )
    })
  }, [filteredOutRemovedTokens, searchValue])

  // Methods
  const filterWatchlist = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    setSearchValue(event.target.value)
  }, [tokenList])

  const findUpdatedTokenInfo = React.useCallback((token: BraveWallet.BlockchainToken) => {
    return updatedTokensList.find((t) => checkIfTokensMatch(t, token))
  }, [updatedTokensList])

  const isUserToken = React.useCallback((token: BraveWallet.BlockchainToken) => {
    return updatedTokensList.some(t => checkIfTokensMatch(t, token))
  }, [updatedTokensList])

  const isAssetSelected = React.useCallback((token: BraveWallet.BlockchainToken): boolean => {
    return (isUserToken(token) && findUpdatedTokenInfo(token)?.visible) ?? false
  }, [isUserToken, findUpdatedTokenInfo])

  const isRemovable = React.useCallback(
    (
      token: BraveWallet.BlockchainToken
    ): boolean => {
      // Native assets should not be removable.
      if (token.contractAddress === '') {
        return false
      }

      // Any NFT should be removable.
      if (
        token.isErc721 ||
        token.isErc1155 ||
        token.isNft
      ) {
        return true
      }

      return !fullTokenListAllChains
        .some(
          each =>
            each.contractAddress.toLowerCase() ===
            token.contractAddress.toLowerCase()
        )
    }, [fullTokenListAllChains])

  const onCheckWatchlistItem = React.useCallback(
    (
      key: string,
      selected: boolean,
      token: BraveWallet.BlockchainToken
    ) => {
      if (isUserToken(token)) {
        const updatedToken = { ...token, visible: selected }
        const tokenIndex =
          updatedTokensList
            .findIndex((t) => checkIfTokensMatch(t, token))
        let newList = [...updatedTokensList]
        newList.splice(tokenIndex, 1, updatedToken)
        setUpdatedTokensList(newList)
        return
      }
      setUpdatedTokensList([...updatedTokensList, token])
    }, [isUserToken, updatedTokensList])

  const toggleShowAddCustomToken = () => setShowAddCustomToken(prev => !prev)

  const onClickSuggestAdd = React.useCallback(() => {
    setTokenContractAddress(searchValue)
    toggleShowAddCustomToken()
  }, [searchValue, toggleShowAddCustomToken])

  const onRemoveAsset = React.useCallback((token: BraveWallet.BlockchainToken) => {
    const filterFn = (t: BraveWallet.BlockchainToken) => !(t.contractAddress.toLowerCase() === token.contractAddress.toLowerCase() && t.tokenId === token.tokenId)
    const newUserList = updatedTokensList.filter(filterFn)
    setUpdatedTokensList(newUserList)
    setRemovedTokensList([token, ...removedTokensList])
  }, [updatedTokensList, filteredTokenList, removedTokensList])

  const onClickDone = React.useCallback(() => {
    onUpdateVisibleAssets(updatedTokensList)
    onClose()
  }, [updatedTokensList, onUpdateVisibleAssets, onClose])

  const onToggleShowNetworkDropdown = React.useCallback(() => {
    setShowNetworkDropDown((prev) => !prev)
  }, [])

  const onSelectAssetsNetwork = React.useCallback((network: BraveWallet.NetworkInfo) => {
    setSelectedNetworkFilter(network)
    setShowNetworkDropDown(false)
  }, [])

  return (
    <PopupModal
      title={showAddCustomToken
        ? getLocale('braveWalletWatchlistAddCustomAsset')
        : getLocale('braveWalletAccountsEditVisibleAssets')
      }
      onClose={onClose}
    >
      <StyledWrapper>
        {(filteredTokenList.length === 0 && searchValue === '') || isLoading ? (
          <LoadingWrapper>
            <LoadIcon />
          </LoadingWrapper>
        ) : (
          <>
            {showAddCustomToken
              ? <AddAsset
                contractAddress={tokenContractAddress}
                onHideForm={toggleShowAddCustomToken}
              />
              : <>
                <NetworkFilterWithSearch
                  searchValue={searchValue}
                  searchPlaceholder={getLocale('braveWalletWatchListSearchPlaceholder')}
                  searchAction={filterWatchlist}
                  searchAutoFocus={true}
                  selectedNetwork={selectedNetworkFilter}
                  onClick={onToggleShowNetworkDropdown}
                  showNetworkDropDown={showNetworkDropDown}
                  onSelectNetwork={onSelectAssetsNetwork}
                />
                {!searchValue.toLowerCase().startsWith('0x') &&
                  <TopRow>
                    <NoAssetButton onClick={toggleShowAddCustomToken}>
                      {getLocale('braveWalletWatchlistAddCustomAsset')}
                    </NoAssetButton>
                  </TopRow>
                }
                {filteredTokenList.length === 0
                  ? <NoAssetRow>
                    {searchValue.toLowerCase().startsWith('0x') ? (
                      <NoAssetButton
                        onClick={onClickSuggestAdd}>{getLocale('braveWalletWatchListSuggestion').replace('$1', searchValue)}</NoAssetButton>
                    ) : (
                      <NoAssetText>{getLocale('braveWalletWatchListNoAsset')} {searchValue}</NoAssetText>
                    )}
                  </NoAssetRow>
                  : <VirtualizedVisibleAssetsList
                    tokenList={filteredTokenList}
                    isRemovable={isRemovable}
                    onRemoveAsset={onRemoveAsset}
                    isAssetSelected={isAssetSelected}
                    onCheckWatchlistItem={onCheckWatchlistItem}
                  />
                }
                <NavButton
                  onSubmit={onClickDone}
                  text={getLocale('braveWalletWatchListDoneButton')}
                  buttonType='primary'
                />
              </>
            }
          </>
        )}
      </StyledWrapper>
    </PopupModal>
  )
}

export default EditVisibleAssetsModal
