// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { eachLimit } from 'async'

// Selectors
import { useUnsafeWalletSelector } from './use-safe-selector'
import { WalletSelectors } from '../selectors'

// Constants
import { BraveWallet } from '../../constants/types'

// Utils
import { stripERC20TokenImageURL } from '../../utils/string-utils'
import { WalletActions } from '../actions'
import { getAssetIdKey, isTokenIdRemoved } from '../../utils/asset-utils'
import {
  useGetUserTokensRegistryQuery,
  useHideOrDeleteTokenMutation,
  useRestoreHiddenTokenMutation
} from '../slices/api.slice'

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

  // redux
  const dispatch = useDispatch()

  // queries
  const { data: userTokensRegistry } = useGetUserTokensRegistryQuery()

  // mutations
  const [hideOrDeleteToken] = useHideOrDeleteTokenMutation()
  const [restoreHiddenToken] = useRestoreHiddenTokenMutation()

  const addNftToDeletedNftsList = React.useCallback(
    async (token: BraveWallet.BlockchainToken) => {
      await hideOrDeleteToken({
        mode: 'delete',
        tokenId: getAssetIdKey(token)
      })
    },
    [hideOrDeleteToken]
  )

  const onAddUserAsset = React.useCallback(
    async (token: BraveWallet.BlockchainToken) => {
      const assetId = getAssetIdKey(token)
      if (
        userTokensRegistry &&
        isTokenIdRemoved(assetId, userTokensRegistry.hiddenTokenIds)
      ) {
        await restoreHiddenToken(assetId)
      } else {
        dispatch(
          WalletActions.addUserAsset({
            ...token,
            logo: stripERC20TokenImageURL(token.logo) || ''
          })
        )
      }
    },
    [restoreHiddenToken, userTokensRegistry]
  )

  const onAddCustomAsset = React.useCallback(
    async (token: BraveWallet.BlockchainToken) => {
      await onAddUserAsset(token)

      // We handle refreshing balances for ERC721 tokens in the
      // addUserAsset handler.
      if (!(token.isErc721 || token.isNft)) {
        dispatch(WalletActions.refreshBalancesAndPriceHistory())
      }
    },
    [onAddUserAsset]
  )

  const onUpdateVisibleAssets = React.useCallback(
    async (updatedTokensList: BraveWallet.BlockchainToken[]) => {
      // Gets a list of all added tokens and adds them to the
      // userVisibleTokensInfo list
      await eachLimit(
        onlyInLeft(updatedTokensList, userVisibleTokensInfo),
        10,
        onAddUserAsset
      )

      // Gets a list of all removed tokens and removes them from the
      // userVisibleTokensInfo list
      await eachLimit(
        onlyInLeft(userVisibleTokensInfo, updatedTokensList),
        10,
        async (token) => {
          dispatch(WalletActions.removeUserAsset(token))
          const tokenId = getAssetIdKey(token)
          if (
            userTokensRegistry &&
            !isTokenIdRemoved(tokenId, userTokensRegistry.hiddenTokenIds)
          ) {
            await hideOrDeleteToken({ mode: 'hide', tokenId })
          }
        }
      )

      // Gets a list of custom tokens and native assets returned from
      // updatedTokensList payload then compares against userVisibleTokensInfo
      // list and updates the tokens visibility if it has changed.
      await eachLimit(
        findTokensWithMismatchedVisibility(
          updatedTokensList,
          userVisibleTokensInfo
        ),
        10,
        async (token) =>
          await dispatch(
            WalletActions.setUserAssetVisible({
              token,
              isVisible: token.visible
            })
          )
      )

      // Refresh Balances, Prices and Price History when done.
      dispatch(WalletActions.refreshBalancesAndPriceHistory())
    },
    [userVisibleTokensInfo, userTokensRegistry, hideOrDeleteToken]
  )

  const makeTokenVisible = React.useCallback(
    async (token: BraveWallet.BlockchainToken) => {
      const foundTokenIdx = userVisibleTokensInfo.findIndex(
        (t) =>
          t.contractAddress.toLowerCase() ===
            token.contractAddress.toLowerCase() && t.chainId === token.chainId
      )

      const updatedTokensList = [...userVisibleTokensInfo]

      // If token is not part of user-visible tokens, add it.
      if (foundTokenIdx === -1) {
        await onUpdateVisibleAssets([...updatedTokensList, token])
        return
      }

      if (userVisibleTokensInfo[foundTokenIdx].visible) {
        return
      }

      // If token is part of user-visible tokens, then:
      //   - toggle visibility for custom tokens
      //   - do nothing for non-custom tokens
      updatedTokensList.splice(foundTokenIdx, 1, { ...token, visible: true })
      await onUpdateVisibleAssets(updatedTokensList)
    },
    [userVisibleTokensInfo, onUpdateVisibleAssets]
  )

  return {
    onUpdateVisibleAssets,
    onAddCustomAsset,
    makeTokenVisible,
    addNftToDeletedNftsList
  }
}

export default useAssetManagement
