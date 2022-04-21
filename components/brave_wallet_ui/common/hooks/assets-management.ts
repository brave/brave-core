// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { SimpleActionCreator, EmptyActionCreator } from 'redux-act'
import {
  BraveWallet
} from '../../constants/types'

import {
  AddUserAssetPayloadType,
  SetUserAssetVisiblePayloadType,
  RemoveUserAssetPayloadType
} from '../constants/action_types'

import { stripERC20TokenImageURL } from '../../utils/string-utils'

export default function useAssetManagement (
  addUserAsset: SimpleActionCreator<AddUserAssetPayloadType>,
  setUserAssetVisible: SimpleActionCreator<SetUserAssetVisiblePayloadType>,
  removeUserAsset: SimpleActionCreator<RemoveUserAssetPayloadType>,
  refreshBalancesPricesAndHistory: EmptyActionCreator,
  fullTokenList: BraveWallet.BlockchainToken[],
  userVisibleTokensInfo: BraveWallet.BlockchainToken[]
) {
  const onlyInLeft = (left: BraveWallet.BlockchainToken[], right: BraveWallet.BlockchainToken[]) =>
    left.filter(leftValue =>
      !right.some(rightValue =>
        leftValue.contractAddress.toLowerCase() === rightValue.contractAddress.toLowerCase() &&
        leftValue.chainId === rightValue.chainId))

  const onAddUserAsset = (token: BraveWallet.BlockchainToken) => {
    addUserAsset({
      token: {
        ...token,
        logo: stripERC20TokenImageURL(token.logo) || ''
      },
      chainId: token?.chainId ?? ''
    })
  }

  const onAddCustomAsset = (token: BraveWallet.BlockchainToken) => {
    onAddUserAsset(token)
    refreshBalancesPricesAndHistory()
  }

  const findVisibleTokenInfo = React.useCallback((token: BraveWallet.BlockchainToken) =>
    userVisibleTokensInfo.find(t =>
      t.contractAddress.toLowerCase() === token.contractAddress.toLowerCase() &&
      t.chainId === token.chainId),
    [userVisibleTokensInfo])

  const onUpdateVisibleAssets = React.useCallback((updatedTokensList: BraveWallet.BlockchainToken[]) => {
    // Gets a list of all added tokens and adds them to the userVisibleTokensInfo list
    onlyInLeft(updatedTokensList, userVisibleTokensInfo)
      .forEach(token => onAddUserAsset(token))

    // Gets a list of all removed tokens and removes them from the userVisibleTokensInfo list
    onlyInLeft(userVisibleTokensInfo, updatedTokensList)
      .forEach(token => removeUserAsset({ token, chainId: token?.chainId ?? '' }))

    // Gets a list of custom tokens and native assets returned from updatedTokensList payload
    // then compares against userVisibleTokensInfo list and updates the tokens visibility if it has changed.
    onlyInLeft(updatedTokensList, fullTokenList)
      .forEach(token => {
        const foundToken = findVisibleTokenInfo(token)
        // Updates token visibility
        if (foundToken?.visible !== token.visible) {
          setUserAssetVisible({ token, chainId: token?.chainId ?? '', isVisible: token.visible })
        }
      })

    // Refreshes Balances, Prices and Price History when done.
    refreshBalancesPricesAndHistory()
  }, [userVisibleTokensInfo])

  return {
    onUpdateVisibleAssets,
    onAddCustomAsset
  }
}
