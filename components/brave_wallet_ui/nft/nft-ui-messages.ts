// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { NFTMetadataReturnType } from '../constants/types'

export const braveWalletOrigin = 'chrome://wallet'
export const braveWalletPanelOrigin = 'chrome://wallet-panel.top-chrome'

export type DisplayMode = 'icon' | 'grid' | 'details'

export const DISPLAY_MODES: DisplayMode[] = ['details', 'grid', 'icon']

export type UpdateNFtMetadataPayload = {
  displayMode: DisplayMode
  icon?: string
  nftMetadata?: NFTMetadataReturnType
  imageCID?: string
}
