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
import { useAssetManagement, useTokenRegistry } from '../../../../common/hooks'
import { useSelector } from 'react-redux'
import { useGetSelectedChainQuery } from '../../../../common/slices/api.slice'

export interface Props {
  onClose: () => void
}

const EditVisibleAssetsModal = ({ onClose }: Props) => {
  // redux
  const userVisibleTokensInfo = useSelector(({ wallet }: { wallet: WalletState }) => wallet.userVisibleTokensInfo)

  // queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()

  // custom hooks
  const {
    onUpdateVisibleAssets
  } = useAssetManagement()
  const { tokenRegistry, fullTokenListAllChains, isLoading } = useTokenRegistry()

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

  // Token list based on selectedNetworkFilter
  const selectedNetworkList: BraveWallet.BlockchainToken[] = React.useMemo(() => {
    if (selectedNetworkFilter.chainId === AllNetworksOption.chainId) {
      return fullTokenListAllChains
    }
    return Object.keys(tokenRegistry).length === 0 ? [] : tokenRegistry[selectedNetworkFilter.chainId]
  }, [tokenRegistry, selectedNetworkFilter.chainId, fullTokenListAllChains, Object.keys(tokenRegistry).length])

  // Users visible tokens based on selectedNetworkFilter
  const userVisibleTokensBySelectedNetwork: BraveWallet.BlockchainToken[] = React.useMemo(() => {
    if (selectedNetworkFilter.chainId === AllNetworksOption.chainId) {
      return userVisibleTokensInfo
    }
    return userVisibleTokensInfo.filter((token) => token.chainId === selectedNetworkFilter.chainId)
  }, [userVisibleTokensInfo, selectedNetworkFilter.chainId, tokenRegistry])

  // Constructed list based on Users Visible Tokens and Full Token List
  const tokenList: BraveWallet.BlockchainToken[] = React.useMemo(() => {
    const userVisibleContracts = isUserVisibleTokensInfoEmpty
      ? []
      : userVisibleTokensBySelectedNetwork.map((token) => token.contractAddress.toLowerCase())

    const fullAssetsListPlusNativeToken = userVisibleContracts.includes('') || !nativeAsset
      ? selectedNetworkList
      : [nativeAsset, ...selectedNetworkList]

    const filteredTokenRegistry = fullAssetsListPlusNativeToken
      .filter((token) => !userVisibleContracts.includes(token?.contractAddress?.toLowerCase()))

    return isUserVisibleTokensInfoEmpty
      ? filteredTokenRegistry
      : [...userVisibleTokensBySelectedNetwork, ...filteredTokenRegistry]
  }, [isUserVisibleTokensInfoEmpty, selectedNetworkList, userVisibleTokensInfo, nativeAsset])

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

  const isCustomToken = React.useCallback((token: BraveWallet.BlockchainToken): boolean => {
    if (token.contractAddress === '') {
      return false
    }

    // Any token with a tokenId should be considered a custom token.
    if (token.tokenId !== '') {
      return true
    }

    return !fullTokenListAllChains
      .some(each => each.contractAddress.toLowerCase() === token.contractAddress.toLowerCase())
  }, [fullTokenListAllChains])

  const addOrRemoveTokenFromList = React.useCallback((selected: boolean, token: BraveWallet.BlockchainToken) => {
    if (selected) {
      return [...updatedTokensList, token]
    }
    return updatedTokensList.filter((t) => t !== token)
  }, [updatedTokensList])

  // Do to a bug, users were able to set a non custom token's
  // visibility to false. We only allow setting visibility to custom tokens
  // to make it easier for users to re-add in the future. This method is added
  // to help the user get un-stuck.
  const findNonCustomTokenWithVisibleFalse = React.useCallback((token: BraveWallet.BlockchainToken) => {
    return userVisibleTokensInfo.some((t) =>
      checkIfTokensMatch(t, token) &&
      !t.visible)
  }, [userVisibleTokensInfo])

  const onCheckWatchlistItem = React.useCallback((key: string, selected: boolean, token: BraveWallet.BlockchainToken, isCustom: boolean) => {
    if (isUserToken(token)) {
      if (isCustom || token.contractAddress === '' || (!isCustom && findNonCustomTokenWithVisibleFalse(token))) {
        const updatedToken = selected ? { ...token, visible: true } : { ...token, visible: false }
        const tokenIndex = updatedTokensList.findIndex((t) => checkIfTokensMatch(t, token))
        let newList = [...updatedTokensList]
        newList.splice(tokenIndex, 1, updatedToken)
        setUpdatedTokensList(newList)
      } else {
        if (token.isErc721) setTokenContractAddress(token.contractAddress)
        setUpdatedTokensList(addOrRemoveTokenFromList(selected, token))
      }
      return
    }
    if (token.isErc721 || token.isNft) {
      setShowAddCustomToken(true)
      setTokenContractAddress(token.contractAddress)
      return
    }
    setUpdatedTokensList(addOrRemoveTokenFromList(selected, token))
  }, [isUserToken, updatedTokensList, addOrRemoveTokenFromList, findNonCustomTokenWithVisibleFalse])

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
                    isCustomToken={isCustomToken}
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
