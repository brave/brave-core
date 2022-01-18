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
  selectedNetwork: BraveWallet.EthereumChain,
  fullTokenList: BraveWallet.BlockchainToken[],
  userVisibleTokensInfo: BraveWallet.BlockchainToken[]
) {
  const onAddUserAsset = (token: BraveWallet.BlockchainToken) => {
    addUserAsset({
      token: {
        ...token,
        logo: stripERC20TokenImageURL(token.logo) || ''
      },
      chainId: selectedNetwork.chainId
    })
  }

  const onAddCustomAsset = (token: BraveWallet.BlockchainToken) => {
    onAddUserAsset(token)
    refreshBalancesPricesAndHistory()
  }

  const findVisibleTokenInfo = (token: BraveWallet.BlockchainToken) => {
    return userVisibleTokensInfo.find((t) => t.contractAddress.toLowerCase() === token.contractAddress.toLowerCase())
  }

  const onUpdateVisibleAssets = React.useCallback((updatedTokensList: BraveWallet.BlockchainToken[]) => {
    // Gets a list of all added tokens and adds them to the userVisibleTokensInfo list
    updatedTokensList.filter((firstItem) =>
      !userVisibleTokensInfo.some((secondItem) =>
        firstItem.contractAddress.toLowerCase() === secondItem.contractAddress.toLowerCase()))
      .forEach((token) => onAddUserAsset(token))

    // Gets a list of all removed tokens and removes them from the userVisibleTokensInfo list
    userVisibleTokensInfo.filter((firstItem) =>
      !updatedTokensList.some((secondItem) =>
        firstItem.contractAddress.toLowerCase() === secondItem.contractAddress.toLowerCase()))
      .forEach((token) => removeUserAsset({ token, chainId: selectedNetwork.chainId }))

    // Gets a list of custom tokens returned from updatedTokensList payload
    // then compares customTokens against userVisibleTokensInfo list and updates the custom tokens visibility if it has changed
    updatedTokensList.filter((firstItem) =>
      !fullTokenList.some((secondItem) =>
        firstItem.contractAddress.toLowerCase() === secondItem.contractAddress.toLowerCase()))
      .forEach((token) => {
        const foundToken = findVisibleTokenInfo(token)
        // Since a networks native token (example 'ETH') can be removed from the the userVisibleTokensInfo list,
        // when it is re-added we handle it as a custom token since it is not part of the token registry.
        // This check here will add it only if it's value 'visible' is returned true
        if (token.contractAddress.toLowerCase() === '' && !foundToken?.visible && token.visible) {
          onAddUserAsset(token)
        }
        // Updates token visibility exluding a networks native token
        if (foundToken?.visible !== token.visible && token.contractAddress.toLowerCase() !== '') {
          setUserAssetVisible({ token, chainId: selectedNetwork.chainId, isVisible: token.visible })
        }
      })

    // Refreshes Balances, Prices and Price History when done.
    refreshBalancesPricesAndHistory()
  }, [userVisibleTokensInfo, selectedNetwork])

  return {
    onUpdateVisibleAssets,
    onAddCustomAsset
  }
}
