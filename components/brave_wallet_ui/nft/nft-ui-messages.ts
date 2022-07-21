// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { loadTimeData } from '../../common/loadTimeData'
import { BraveWallet, NFTMetadataReturnType } from '../constants/types'

const walletOrigin = loadTimeData.getString('braveWalletBridgeUrl')
// remove trailing /
export const braveWalletOrigin = walletOrigin.endsWith('/') ? walletOrigin.slice(0, -1) : walletOrigin

export const enum NftUiCommand {
  UpdateLoading = 'update-loading',
  UpdateSelectedAsset = 'update-selected-asset',
  UpdateNFTMetadata = 'update-nft-metadata',
  UpdateTokenNetwork = 'update-token-network'
}

export type CommandMessage = {
  command: NftUiCommand
}

export type UpdateLoadingMessage = CommandMessage & {
  payload: boolean
}

export type UpdateSelectedAssetMessage = CommandMessage & {
  payload: BraveWallet.BlockchainToken
}

export type UpdateNFtMetadataMessage = CommandMessage & {
  payload: NFTMetadataReturnType
}

export type UpdateTokenNetworkMessage = CommandMessage & {
  payload: BraveWallet.NetworkInfo
}

export const sendMessageToNftUiFrame = (targetWindow: Window | null, message: CommandMessage) => {
  if (targetWindow) {
    // Using targetOrigin '*' is not recommended for security reasons
    // The challenge with the current use case is that brave wallet's origin is chrome://wallet
    // and the iframe's origin is chrome-untrusted://nft-display
    // the two origins do not share the same scheme as required by window.postMessage
    // https://developer.mozilla.org/en-US/docs/Web/API/Window/postMessage
    // the solution I have implemented is to validate the origin of the message before
    // proceeding to with the logic
    // if alternatives exist, we can refactor.
    targetWindow.postMessage(message, '*')
  }
}
