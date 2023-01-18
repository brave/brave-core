// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'

// Constants
import { BraveWallet, WalletState } from '../../constants/types'

// Utils
import { stripERC20TokenImageURL } from '../../utils/string-utils'
import { WalletActions } from '../actions'

const onlyInLeft = (left: BraveWallet.BlockchainToken[], right: BraveWallet.BlockchainToken[]) =>
  left.filter(leftValue =>
    !right.some(rightValue =>
      leftValue.contractAddress.toLowerCase() === rightValue.contractAddress.toLowerCase() &&
      leftValue.chainId === rightValue.chainId && leftValue.tokenId === rightValue.tokenId))

const findTokensWithMismatchedVisibility = (left: BraveWallet.BlockchainToken[], right: BraveWallet.BlockchainToken[]) =>
  left.filter(leftValue =>
    right.some(rightValue =>
      leftValue.contractAddress.toLowerCase() === rightValue.contractAddress.toLowerCase() &&
      leftValue.chainId === rightValue.chainId &&
      leftValue.tokenId === rightValue.tokenId &&
      leftValue.visible !== rightValue.visible))

export default function useAssetManagement () {
  // redux
  const {
    userVisibleTokensInfo
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)
  const dispatch = useDispatch()

  const onAddUserAsset = (token: BraveWallet.BlockchainToken) => {
    dispatch(WalletActions.addUserAsset({
      ...token,
      logo: stripERC20TokenImageURL(token.logo) || ''
    }))
  }

  const onAddCustomAsset = (token: BraveWallet.BlockchainToken) => {
    onAddUserAsset(token)

    // We handle refreshing balances for ERC721 tokens in the addUserAsset handler.
    if (!(token.isErc721 || token.isNft)) {
      dispatch(WalletActions.refreshBalancesAndPriceHistory())
    }
  }

  const onUpdateVisibleAssets = React.useCallback((updatedTokensList: BraveWallet.BlockchainToken[]) => {
    // Gets a list of all added tokens and adds them to the userVisibleTokensInfo list
    onlyInLeft(updatedTokensList, userVisibleTokensInfo)
      .forEach(token => onAddUserAsset(token))

    // Gets a list of all removed tokens and removes them from the userVisibleTokensInfo list
    onlyInLeft(userVisibleTokensInfo, updatedTokensList)
      .forEach(token => dispatch(WalletActions.removeUserAsset(token)))

    // Gets a list of custom tokens and native assets returned from updatedTokensList payload
    // then compares against userVisibleTokensInfo list and updates the tokens visibility if it has changed.
    findTokensWithMismatchedVisibility(updatedTokensList, userVisibleTokensInfo)
      .forEach(token => dispatch(WalletActions.setUserAssetVisible({ token, isVisible: token.visible })))

    // Refresh Balances, Prices and Price History when done.
    dispatch(WalletActions.refreshBalancesAndPriceHistory())
  }, [userVisibleTokensInfo])

  const makeTokenVisible = React.useCallback((token: BraveWallet.BlockchainToken) => {
    const foundTokenIdx = userVisibleTokensInfo.findIndex(t =>
      t.contractAddress.toLowerCase() === token.contractAddress.toLowerCase() &&
      t.chainId === token.chainId)

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
  }, [userVisibleTokensInfo, onUpdateVisibleAssets])

  return {
    onUpdateVisibleAssets,
    onAddCustomAsset,
    makeTokenVisible
  }
}
