// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { background } from 'ethereum-blockies'

// Constants
import { BraveWallet } from '../../../constants/types'

// Utils
import {
  stripERC20TokenImageURL,
  isRemoteImageURL,
  isValidIconExtension,
  addIpfsGateway,
  isIpfs,
  isDataURL
} from '../../../utils/string-utils'

// Styled components
import { IconWrapper, PlaceholderText } from './style'

// Options
import { getNetworkLogo } from '../../../options/asset-options'

interface Config {
  size: 'big' | 'small'
  marginLeft?: number
  marginRight?: number
}

interface Props {
  asset: BraveWallet.BlockchainToken | undefined
  network: BraveWallet.NetworkInfo | undefined
}

function withPlaceholderIcon (WrappedComponent: React.ComponentType<any>, config: Config) {
  const {
    size,
    marginLeft,
    marginRight
  } = config

  return function (props: Props) {
    const { asset, network } = props

    if (!asset || !network) {
      return null
    }

    const networkLogo = getNetworkLogo(network)

    const isNativeAsset = React.useMemo(() =>
      asset.symbol.toLowerCase() === network.symbol.toLowerCase(),
      [network.symbol, asset.symbol]
    )

    const isNonFungibleToken = React.useMemo(() => asset.isNft || asset.isErc721, [asset.isNft, asset.isErc721])

    const tokenImageURL = stripERC20TokenImageURL(asset.logo)
    const isRemoteURL = isRemoteImageURL(tokenImageURL)
    const isStorybook = asset.logo.startsWith('static/media/components/brave_wallet_ui/')

    const isValidIcon = React.useMemo(() => {
      if (isRemoteURL || isDataURL(asset.logo)) {
        return tokenImageURL?.includes('data:image/') || isIpfs(tokenImageURL) || isNonFungibleToken ? true : isValidIconExtension(new URL(asset.logo).pathname)
      }
      if (isStorybook) {
        return true
      }
      return false
    }, [isRemoteURL, tokenImageURL, asset.logo, isStorybook])

    const needsPlaceholder = isNativeAsset
      ? (tokenImageURL === '' || !isValidIcon) && networkLogo === ''
      : tokenImageURL === '' || !isValidIcon

    const bg = React.useMemo(() => {
      if (needsPlaceholder) {
        return background({ seed: asset.contractAddress ? asset.contractAddress.toLowerCase() : asset.name })
      }
    }, [needsPlaceholder, asset.contractAddress, asset.name])

    const remoteImage = React.useMemo(() => {
      if (isRemoteURL) {
        return `chrome://image?${addIpfsGateway(tokenImageURL)}`
      }
      return ''
    }, [isRemoteURL, tokenImageURL])

    if (needsPlaceholder) {
      return (
        <IconWrapper
          panelBackground={bg}
          isPlaceholder={true}
          size={size}
          marginLeft={marginLeft ?? 0}
          marginRight={marginRight ?? 0}
        >
          <PlaceholderText size={size}>{asset.symbol.charAt(0)}</PlaceholderText>
        </IconWrapper>
      )
    }

    return (
      <IconWrapper
        isPlaceholder={false}
        size={size}
        marginLeft={marginLeft ?? 0}
        marginRight={marginRight ?? 0}
      >
        <WrappedComponent
          icon={
            isNativeAsset && networkLogo
              ? networkLogo
              : isRemoteURL ? remoteImage : asset.logo
          }
        />
      </IconWrapper>
    )
  }
}

export default withPlaceholderIcon
