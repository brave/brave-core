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
  isDataURL,
  isIpfs
} from '../../../utils/string-utils'

// Styled components
import { IconWrapper, PlaceholderText } from './style'

// Options
import { makeNativeAssetLogo } from '../../../options/asset-options'
import { translateToNftGateway } from '../../../common/async/lib'

interface Config {
  size: 'big' | 'medium' | 'small'
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

    const nativeAssetLogo = makeNativeAssetLogo(network.symbol, network.chainId)

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
      ? (tokenImageURL === '' || !isValidIcon) && nativeAssetLogo === ''
      : tokenImageURL === '' || !isValidIcon

    const bg = React.useMemo(() => {
      if (needsPlaceholder) {
        return background({ seed: asset.contractAddress ? asset.contractAddress.toLowerCase() : asset.name })
      }
    }, [needsPlaceholder, asset.contractAddress, asset.name])

    const [ipfsUrl, setIpfsUrl] = React.useState<string>()

    // memos
    React.useEffect(() => {
      let ignore = false
      translateToNftGateway(tokenImageURL).then(
        (v) => { if (!ignore) setIpfsUrl(v) })
      return () => {
        ignore = true
      }
    }, [tokenImageURL])

    const remoteImage = React.useMemo(() => {
      if (isRemoteURL) {
        return `chrome://image?${ipfsUrl}`
      }
      return ''
    }, [isRemoteURL, tokenImageURL, ipfsUrl])

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
            isNativeAsset && nativeAssetLogo
              ? nativeAssetLogo
              : isRemoteURL ? remoteImage : asset.logo
          }
        />
      </IconWrapper>
    )
  }
}

export default withPlaceholderIcon
