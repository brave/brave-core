// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { loadTimeData } from '../../common/loadTimeData'
import { BraveWallet, NFTMetadataReturnType } from '../constants/types'

const nftDisplayOrigin = loadTimeData.getString('braveWalletNftBridgeUrl')
// remove trailing /
export const braveNftDisplayOrigin = nftDisplayOrigin.endsWith('/') ? nftDisplayOrigin.slice(0, -1) : nftDisplayOrigin

export const braveWalletOrigin = 'chrome://wallet'
export const braveWalletPanelOrigin = 'chrome://wallet-panel.top-chrome'

export const enum NftUiCommand {
  UpdateLoading = 'update-loading',
  UpdateSelectedAsset = 'update-selected-asset',
  UpdateNFTMetadata = 'update-nft-metadata',
  UpdateNFTMetadataError = 'update-nft-metadata-error',
  UpdateTokenNetwork = 'update-token-network',
  ToggleNftModal = 'toggle-nft-modal'
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

export type DisplayMode =
| 'icon'
| 'grid'
| 'details'

export type UpdateNFtMetadataMessage = CommandMessage & {
  payload: {
    displayMode: DisplayMode
    icon?: string
    nftMetadata?: NFTMetadataReturnType
  }
}

export type UpdateNFtMetadataErrorMessage = CommandMessage & {
  payload: {
    displayMode: DisplayMode
    error: string | undefined
  }
}

export type UpdateTokenNetworkMessage = CommandMessage & {
  payload: BraveWallet.NetworkInfo
}

export type ToggleNftModal = CommandMessage & {
  payload: boolean
}

export const sendMessageToNftUiFrame = (targetWindow: Window | null, message: CommandMessage) => {
  if (targetWindow) {
    targetWindow.postMessage(message, braveNftDisplayOrigin)
  }
}

export const sendMessageToWalletUi = (targetWindow: Window | null, message: CommandMessage) => {
  if (targetWindow) {
    targetWindow.postMessage(message, braveWalletOrigin)
  }
}
