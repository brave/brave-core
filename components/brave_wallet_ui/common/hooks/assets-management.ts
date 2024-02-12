// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'

// Selectors
import { useUnsafeWalletSelector } from './use-safe-selector'
import { WalletSelectors } from '../selectors'

// Constants
import { BraveWallet } from '../../constants/types'

// Utils
import { stripERC20TokenImageURL } from '../../utils/string-utils'
import { WalletActions } from '../actions'
import { LOCAL_STORAGE_KEYS } from '../constants/local-storage-keys'
import { getAssetIdKey } from '../../utils/asset-utils'

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

const findTokensWithMismatchedVisibility = (
  left: BraveWallet.BlockchainToken[],
  right: BraveWallet.BlockchainToken[]
) =>
  left.filter((leftValue) =>
    right.some(
      (rightValue) =>
        leftValue.contractAddress.toLowerCase() ===
          rightValue.contractAddress.toLowerCase() &&
        leftValue.chainId === rightValue.chainId &&
        leftValue.tokenId === rightValue.tokenId &&
        leftValue.visible !== rightValue.visible
    )
  )

export function useAssetManagement() {
  // selectors
  const userVisibleTokensInfo = useUnsafeWalletSelector(
    WalletSelectors.userVisibleTokensInfo
  )
  const removedFungibleTokenIds = useUnsafeWalletSelector(
    WalletSelectors.removedFungibleTokenIds
  )
  const removedNonFungibleTokenIds = useUnsafeWalletSelector(
    WalletSelectors.removedNonFungibleTokenIds
  )
  const deletedNonFungibleTokenIds = useUnsafeWalletSelector(
    WalletSelectors.deletedNonFungibleTokenIds
  )

  // redux
  const dispatch = useDispatch()

  const tokenIsSetAsRemovedInLocalStorage = React.useCallback(
    (token: BraveWallet.BlockchainToken) => {
      const assetId = getAssetIdKey(token)
      return token.isNft || token.isErc1155 || token.isErc721
        ? removedNonFungibleTokenIds.includes(assetId)
        : removedFungibleTokenIds.includes(assetId)
    },
    [removedNonFungibleTokenIds, removedFungibleTokenIds]
  )

  const addOrRemoveTokenInLocalStorage = React.useCallback(
    (token: BraveWallet.BlockchainToken, addOrRemove: 'add' | 'remove') => {
      if (token.contractAddress === '') {
        // prevent permanently removing native tokens
        return
      }

      const assetId = getAssetIdKey(token)
      const isNFT = token.isNft || token.isErc1155 || token.isErc721
      const removedList = isNFT
        ? removedNonFungibleTokenIds
        : removedFungibleTokenIds
      const localStorageKey = isNFT
        ? LOCAL_STORAGE_KEYS.USER_REMOVED_NON_FUNGIBLE_TOKEN_IDS
        : LOCAL_STORAGE_KEYS.USER_REMOVED_FUNGIBLE_TOKEN_IDS

      // add assetId if it is not in the array
      if (addOrRemove === 'remove') {
        const newList = [...removedList, assetId]
        // update state
        if (isNFT) {
          dispatch(WalletActions.setRemovedNonFungibleTokenIds(newList))
        } else {
          dispatch(WalletActions.setRemovedFungibleTokenIds(newList))
        }
        // persist array
        localStorage.setItem(localStorageKey, JSON.stringify(newList))
      }

      // add assetId if it is not in the array
      if (addOrRemove === 'add') {
        const newList = removedList.filter((id) => id !== assetId)
        // update state
        if (isNFT) {
          dispatch(WalletActions.setRemovedNonFungibleTokenIds(newList))
        } else {
          dispatch(WalletActions.setRemovedFungibleTokenIds(newList))
        }
        // persist array
        localStorage.setItem(localStorageKey, JSON.stringify(newList))
      }
    },
    [removedNonFungibleTokenIds, removedFungibleTokenIds]
  )

  const addNftToDeletedNftsList = React.useCallback(
    (token: BraveWallet.BlockchainToken) => {
      const assetId = getAssetIdKey(token)
      const newList = [...deletedNonFungibleTokenIds, assetId]
      // update state
      dispatch(WalletActions.setDeletedNonFungibleTokenIds(newList))
      // persist array
      localStorage.setItem(
        LOCAL_STORAGE_KEYS.USER_DELETED_NON_FUNGIBLE_TOKEN_IDS,
        JSON.stringify(newList)
      )
    },
    [deletedNonFungibleTokenIds]
  )

  const onAddUserAsset = React.useCallback(
    (token: BraveWallet.BlockchainToken) => {
      if (tokenIsSetAsRemovedInLocalStorage(token)) {
        addOrRemoveTokenInLocalStorage(token, 'add')
      } else {
        dispatch(
          WalletActions.addUserAsset({
            ...token,
            logo: stripERC20TokenImageURL(token.logo) || ''
          })
        )
      }
    },
    [addOrRemoveTokenInLocalStorage, tokenIsSetAsRemovedInLocalStorage]
  )

  const onAddCustomAsset = React.useCallback(
    (token: BraveWallet.BlockchainToken) => {
      onAddUserAsset(token)

      // We handle refreshing balances for ERC721 tokens in the
      // addUserAsset handler.
      if (!(token.isErc721 || token.isNft)) {
        dispatch(WalletActions.refreshBalancesAndPriceHistory())
      }
    },
    [onAddUserAsset]
  )

  const onUpdateVisibleAssets = React.useCallback(
    (updatedTokensList: BraveWallet.BlockchainToken[]) => {
      // Gets a list of all added tokens and adds them to the
      // userVisibleTokensInfo list
      onlyInLeft(updatedTokensList, userVisibleTokensInfo).forEach((token) =>
        onAddUserAsset(token)
      )

      // Gets a list of all removed tokens and removes them from the
      // userVisibleTokensInfo list
      onlyInLeft(userVisibleTokensInfo, updatedTokensList).forEach((token) => {
        dispatch(WalletActions.removeUserAsset(token))
        if (!tokenIsSetAsRemovedInLocalStorage(token)) {
          addOrRemoveTokenInLocalStorage(token, 'remove')
        }
      })

      // Gets a list of custom tokens and native assets returned from
      // updatedTokensList payload then compares against userVisibleTokensInfo
      // list and updates the tokens visibility if it has changed.
      findTokensWithMismatchedVisibility(
        updatedTokensList,
        userVisibleTokensInfo
      ).forEach((token) =>
        dispatch(
          WalletActions.setUserAssetVisible({ token, isVisible: token.visible })
        )
      )

      // Refresh Balances, Prices and Price History when done.
      dispatch(WalletActions.refreshBalancesAndPriceHistory())
    },
    [userVisibleTokensInfo, addOrRemoveTokenInLocalStorage]
  )

  const makeTokenVisible = React.useCallback(
    (token: BraveWallet.BlockchainToken) => {
      const foundTokenIdx = userVisibleTokensInfo.findIndex(
        (t) =>
          t.contractAddress.toLowerCase() ===
            token.contractAddress.toLowerCase() && t.chainId === token.chainId
      )

      const updatedTokensList = [...userVisibleTokensInfo]

      // If token is not part of user-visible tokens, add it.
      if (foundTokenIdx === -1) {
        return onUpdateVisibleAssets([...updatedTokensList, token])
      }

      if (userVisibleTokensInfo[foundTokenIdx].visible) {
        return
      }

      // If token is part of user-visible tokens, then:
      //   - toggle visibility for custom tokens
      //   - do nothing for non-custom tokens
      updatedTokensList.splice(foundTokenIdx, 1, { ...token, visible: true })
      onUpdateVisibleAssets(updatedTokensList)
    },
    [userVisibleTokensInfo, onUpdateVisibleAssets]
  )

  return {
    onUpdateVisibleAssets,
    onAddCustomAsset,
    makeTokenVisible,
    addOrRemoveTokenInLocalStorage,
    addNftToDeletedNftsList
  }
}

export default useAssetManagement
