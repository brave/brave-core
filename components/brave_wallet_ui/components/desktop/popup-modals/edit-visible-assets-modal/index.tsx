// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useLocation } from 'react-router-dom'

// types
import { BraveWallet, WalletRoutes } from '../../../../constants/types'

// options
import { AllNetworksOption } from '../../../../options/network-filter-options'
import { EditVisibleAssetsOptions } from '../../../../options/nav-options'

// utils
import { getLocale } from '../../../../../common/locale'
import { getAssetIdKey } from '../../../../utils/asset-utils'

// components
import {
  PopupModal //
} from '../index'
import { VirtualizedVisibleAssetsList } from './virtualized-visible-assets-list'
import { AddAsset } from '../../add-asset/add-asset'
import {
  SegmentedControl //
} from '../../../shared/segmented_control/segmented_control'
import { SearchBar } from '../../../shared/search-bar'
import { NetworkFilterSelector } from '../../network-filter-selector'

// Styled Components
import {
  LoadIcon,
  LoadingWrapper,
  NoAssetButton,
  StyledWrapper,
  ButtonRow,
  ListWrapper,
  AddButtonText,
  AddIcon,
  InfoIcon,
  EmptyStateWrapper,
  TitleRow
} from './style'
import {
  Column,
  HorizontalSpace,
  Row,
  Text,
  VerticalSpace,
  LeoSquaredButton
} from '../../../shared/style'
import { PaddedRow } from '../style'

// hooks
import {
  useAddUserTokenMutation,
  useGetTokensRegistryQuery,
  useGetUserTokensRegistryQuery,
  useRemoveUserTokenMutation,
  useUpdateUserAssetVisibleMutation
} from '../../../../common/slices/api.slice'
import {
  blockchainTokenEntityAdaptorInitialState,
  selectAllBlockchainTokensFromQueryResult,
  selectAllUserAssetsFromQueryResult
} from '../../../../common/slices/entities/blockchain-token.entity'
import {
  useGetCustomAssetSupportedNetworks //
} from '../../../../common/hooks/use_get_custom_asset_supported_networks'
import { eachLimit } from 'async'

export interface Props {
  onClose: () => void
}

const onlyInLeft = (
  left: BraveWallet.BlockchainToken[],
  right: BraveWallet.BlockchainToken[]
) =>
  left.filter(
    (leftValue) =>
      !right.some(
        (rightValue) =>
          leftValue.contractAddress.toLowerCase() ===
            rightValue.contractAddress.toLowerCase() &&
          leftValue.chainId === rightValue.chainId &&
          leftValue.tokenId === rightValue.tokenId
      )
  )

export const EditVisibleAssetsModal = ({ onClose }: Props) => {
  // routing
  const { hash } = useLocation()

  // queries
  const networkList = useGetCustomAssetSupportedNetworks()

  const { knownTokenListAllChains, isLoading } = useGetTokensRegistryQuery(
    undefined,
    {
      selectFromResult: (res) => ({
        isLoading: res.isLoading,
        knownTokenListAllChains: selectAllBlockchainTokensFromQueryResult(res)
      })
    }
  )
  const { userTokens, userTokensRegistry } = useGetUserTokensRegistryQuery(
    undefined,
    {
      selectFromResult: (res) => ({
        userTokensRegistry:
          res.data ?? blockchainTokenEntityAdaptorInitialState,
        userTokens: selectAllUserAssetsFromQueryResult(res)
      })
    }
  )

  // mutations
  const [updateUserAssetVisible] = useUpdateUserAssetVisibleMutation()
  const [addUserToken] = useAddUserTokenMutation()
  const [removeUserToken] = useRemoveUserTokenMutation()

  // Token List States
  const [searchValue, setSearchValue] = React.useState<string>('')
  const [tokenContractAddress, setTokenContractAddress] =
    React.useState<string>('')
  const [selectedNetworkFilter, setSelectedNetworkFilter] =
    React.useState<BraveWallet.NetworkInfo>(AllNetworksOption)

  // token modification states
  const [tokensToAdd, setTokensToAdd] = React.useState<
    BraveWallet.BlockchainToken[]
  >([])
  const [tokenIdsToRemove, setTokenIdsToRemove] = React.useState<string[]>([])
  const [tokenIdsToUpdateVisibility, setTokenIdsToUpdateVisibility] =
    React.useState<string[]>([])

  // Modal UI States
  const [showAddCustomToken, setShowAddCustomToken] =
    React.useState<boolean>(false)

  // Memos
  /**
   * known tokens that are:
   * - not in the user assets list
   * - pending addition to the user assets list
   */
  const availableAssets = React.useMemo(() => {
    return onlyInLeft(knownTokenListAllChains, userTokens.concat(tokensToAdd))
  }, [knownTokenListAllChains, userTokens, tokensToAdd])

  /**
   * This list to save or discard
   */
  const pendingUserAssetsList = React.useMemo(() => {
    return tokenIdsToRemove.length
      ? userTokens
          .filter((t) => !tokenIdsToRemove.includes(getAssetIdKey(t)))
          .concat(tokensToAdd)
      : userTokens.concat(tokensToAdd)
  }, [tokenIdsToRemove, userTokens, tokensToAdd])

  const myAssetsOrAvailableAssets = React.useMemo(() => {
    return hash === WalletRoutes.AvailableAssetsHash
      ? availableAssets
      : pendingUserAssetsList
  }, [hash, availableAssets, pendingUserAssetsList])

  /** Token list based on selectedNetworkFilter */
  const tokenListForSelectedNetworks: BraveWallet.BlockchainToken[] =
    React.useMemo(() => {
      if (selectedNetworkFilter.chainId === AllNetworksOption.chainId) {
        return myAssetsOrAvailableAssets
      }
      return myAssetsOrAvailableAssets.filter(
        (token) =>
          token.chainId === selectedNetworkFilter.chainId &&
          token.coin === selectedNetworkFilter.coin
      )
    }, [selectedNetworkFilter, myAssetsOrAvailableAssets])

  /** User tokens sorted by visibility */
  const sortedTokenListForSelectedNetworks: BraveWallet.BlockchainToken[] =
    React.useMemo(() => {
      return [...tokenListForSelectedNetworks].sort(
        (a, b) => Number(b.visible) - Number(a.visible)
      )
    }, [tokenListForSelectedNetworks])

  /** Filtered token list based on search value */
  const tokenListSearchResults = React.useMemo(() => {
    if (searchValue === '') {
      return sortedTokenListForSelectedNetworks
    }

    const searchValueLower = searchValue.toLowerCase()

    return sortedTokenListForSelectedNetworks.filter((item) => {
      return (
        item.name.toLowerCase().startsWith(searchValueLower) ||
        item.symbol.toLowerCase().startsWith(searchValueLower) ||
        item.contractAddress.toLowerCase() === searchValueLower
      )
    })
  }, [sortedTokenListForSelectedNetworks, searchValue])

  // Methods
  const updateSearchValue = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      setSearchValue(event.target.value)
    },
    []
  )

  const onCheckWatchlistItem = React.useCallback(
    (token: BraveWallet.BlockchainToken) => {
      setTokensToAdd((prev) => prev.concat(token))
    },
    []
  )

  const toggleShowAddCustomToken = React.useCallback(
    () => setShowAddCustomToken((prev) => !prev),
    []
  )

  const onClickSuggestAdd = React.useCallback(() => {
    setTokenContractAddress(searchValue)
    toggleShowAddCustomToken()
  }, [searchValue, toggleShowAddCustomToken])

  const onRemoveAsset = React.useCallback(
    (token: BraveWallet.BlockchainToken) => {
      setTokenIdsToRemove((prev) => prev.concat(getAssetIdKey(token)))
    },
    []
  )

  const onClickAssetVisibilityToggle = React.useCallback(
    (token: BraveWallet.BlockchainToken) => {
      const assetId = getAssetIdKey(token)
      // add or remove the id to the list of pending visibility updates
      setTokenIdsToUpdateVisibility((prev) => {
        return prev.includes(assetId)
          ? prev.filter((tId) => tId !== assetId)
          : prev.concat(assetId)
      })
    },
    []
  )

  const onClickDone = React.useCallback(async () => {
    // add new tokens to the userVisibleTokensInfo list
    await eachLimit(tokensToAdd, 10, async (token) => {
      await addUserToken(token).unwrap()
    })

    // remove select tokens from the userVisibleTokensInfo list
    await eachLimit(tokenIdsToRemove, 10, async (tokenId) => {
      await removeUserToken(tokenId).unwrap()
    })

    // update visibility of tokens in the userVisibleTokensInfo list
    await eachLimit(tokenIdsToUpdateVisibility, 10, async (tokenId) => {
      const token = userTokensRegistry.entities[tokenId]
      if (!token) {
        return
      }

      await updateUserAssetVisible({
        token,
        isVisible: !token.visible
      }).unwrap()
    })

    onClose()
  }, [
    userTokensRegistry,
    tokensToAdd,
    tokenIdsToRemove,
    tokenIdsToUpdateVisibility,
    onClose,
    addUserToken,
    removeUserToken,
    updateUserAssetVisible
  ])

  const onSelectAssetsNetwork = React.useCallback(
    (network: BraveWallet.NetworkInfo) => {
      setSelectedNetworkFilter(network)
    },
    []
  )

  const assetHasPendingVisibilityChange = React.useCallback(
    (token: BraveWallet.BlockchainToken) => {
      return tokenIdsToUpdateVisibility.includes(getAssetIdKey(token))
    },
    [tokenIdsToUpdateVisibility]
  )

  return (
    <PopupModal
      title={
        showAddCustomToken
          ? getLocale('braveWalletWatchlistAddCustomAsset')
          : getLocale('braveWalletAccountsEditVisibleAssets')
      }
      onClose={onClose}
      width='500px'
      height={showAddCustomToken ? 'unset' : '90vh'}
    >
      {!showAddCustomToken && (
        <>
          <PaddedRow
            margin='12px 0px'
            marginBottom={16}
          >
            <SegmentedControl navOptions={EditVisibleAssetsOptions} />
          </PaddedRow>
          <TitleRow
            marginBottom={8}
            justifyContent='space-between'
          >
            <Text
              textSize='16px'
              textColor='text03'
              isBold={true}
            >
              {hash === WalletRoutes.AvailableAssetsHash
                ? getLocale('braveWalletAvailableAssets')
                : getLocale('braveWalletMyAssets')}
            </Text>
            {hash === WalletRoutes.AvailableAssetsHash && (
              <NoAssetButton onClick={toggleShowAddCustomToken}>
                {getLocale('braveWalletWatchlistAddCustomAsset')}
              </NoAssetButton>
            )}
          </TitleRow>
          <PaddedRow marginBottom={8}>
            <SearchBar
              placeholder={getLocale('braveWalletSearchText')}
              action={updateSearchValue}
              autoFocus={true}
              value={searchValue}
              isV2={true}
            />
            <HorizontalSpace space='16px' />
            <NetworkFilterSelector
              networkListSubset={
                hash === WalletRoutes.AvailableAssetsHash
                  ? networkList
                  : undefined
              }
              onSelectNetwork={onSelectAssetsNetwork}
              selectedNetwork={selectedNetworkFilter}
              isV2={true}
              dropdownPosition='right'
            />
          </PaddedRow>
        </>
      )}
      {showAddCustomToken ? (
        <AddAsset
          contractAddress={tokenContractAddress}
          onHideForm={toggleShowAddCustomToken}
        />
      ) : (
        <StyledWrapper>
          {(myAssetsOrAvailableAssets.length === 0 && searchValue === '') ||
          isLoading ? (
            <LoadingWrapper>
              <LoadIcon />
            </LoadingWrapper>
          ) : (
            <>
              {tokenListSearchResults.length === 0 ? (
                <EmptyStateWrapper>
                  <Column>
                    <InfoIcon />
                    <Text
                      textSize='16px'
                      textColor='text01'
                      isBold={true}
                    >
                      {getLocale('braveWalletAssetNotFound')}
                    </Text>
                    <VerticalSpace space='12px' />
                    <Text
                      textSize='14px'
                      textColor='text03'
                      isBold={false}
                    >
                      {getLocale('braveWalletDidntFindAssetInList')}
                    </Text>
                    <VerticalSpace space='16px' />
                    <LeoSquaredButton
                      onClick={
                        searchValue.toLowerCase().startsWith('0x')
                          ? onClickSuggestAdd
                          : toggleShowAddCustomToken
                      }
                      kind='outline'
                    >
                      <Row width='unset'>
                        <AddIcon />
                        <AddButtonText>
                          {getLocale('braveWalletWatchlistAddCustomAsset')}
                        </AddButtonText>
                      </Row>
                    </LeoSquaredButton>
                  </Column>
                </EmptyStateWrapper>
              ) : (
                <ListWrapper>
                  <VirtualizedVisibleAssetsList
                    tokenList={tokenListSearchResults}
                    onRemoveAsset={onRemoveAsset}
                    isAssetSelected={assetHasPendingVisibilityChange}
                    onCheckWatchlistItem={
                      hash === WalletRoutes.AvailableAssetsHash
                        ? onCheckWatchlistItem
                        : onClickAssetVisibilityToggle
                    }
                    onClickAddCustomAsset={toggleShowAddCustomToken}
                  />
                </ListWrapper>
              )}
            </>
          )}
        </StyledWrapper>
      )}
      {!showAddCustomToken && (
        <ButtonRow>
          <LeoSquaredButton
            onClick={onClose}
            kind='outline'
          >
            {getLocale('braveWalletButtonCancel')}
          </LeoSquaredButton>
          <HorizontalSpace space='16px' />
          <LeoSquaredButton onClick={onClickDone}>
            {getLocale('braveWalletButtonSaveChanges')}
          </LeoSquaredButton>
        </ButtonRow>
      )}
    </PopupModal>
  )
}

export default EditVisibleAssetsModal
