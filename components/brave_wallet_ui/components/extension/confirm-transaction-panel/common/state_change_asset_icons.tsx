// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// components
import { withPlaceholderIcon } from '../../../shared/create-placeholder-icon'
import { NftIcon } from '../../../shared/nft-icon/nft-icon'

// style
import { AssetIconFactory, AssetIconProps } from '../../../shared/style'

const assetIconSizeProps = {
  width: '24px',
  height: 'auto'
}
const AssetIcon = AssetIconFactory<AssetIconProps>(assetIconSizeProps)

const ICON_CONFIG = { size: 'small', marginLeft: 0, marginRight: 8 } as const
export const NFT_ICON_STYLE: React.CSSProperties = { width: 24, maxHeight: 24 }
export const AssetIconWithPlaceholder = withPlaceholderIcon(
  AssetIcon,
  ICON_CONFIG
)
export const NftAssetIconWithPlaceholder = withPlaceholderIcon(
  NftIcon,
  ICON_CONFIG
)
