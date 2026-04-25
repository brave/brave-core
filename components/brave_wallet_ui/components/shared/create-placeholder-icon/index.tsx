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
  isRemoteImageURL,
  isValidIconExtension,
  isDataURL,
  isComponentInStorybook,
} from '../../../utils/string-utils'
import { isNativeAsset } from '../../../utils/asset-utils'

// Styled components
import {
  AssetIconSizes,
  IconWrapper,
  PlaceholderText,
  NFTPlaceholderWrapper,
  NFTPlacholderIcon,
} from './style'

// Options
import { makeNativeAssetLogo } from '../../../options/asset-options'

interface Config {
  size: AssetIconSizes
  marginLeft?: number
  marginRight?: number
}

export type IconAsset = Pick<
  BraveWallet.BlockchainToken,
  | 'chainId'
  | 'contractAddress'
  | 'isErc721'
  | 'isNft'
  | 'logo'
  | 'name'
  | 'symbol'
  | 'isShielded'
>

interface Props {
  asset: IconAsset | undefined
}

const isStorybook = isComponentInStorybook()

export function withPlaceholderIcon<
  P extends {
    icon?: string
  } & JSX.IntrinsicAttributes,
  PROPS_FOR_FUNCTION = Omit<P, 'icon' | 'onLoad' | 'onError'>,
>(
  // ignore "onLoad" and "onError" prop differences
  // since they are handled internally
  WrappedComponent: React.ComponentType<PROPS_FOR_FUNCTION & { icon?: string }>,
  config: Config,
): (props: Props & PROPS_FOR_FUNCTION) => JSX.Element | null {
  const { size, marginLeft, marginRight } = config

  return function (funcProps: Props & PROPS_FOR_FUNCTION) {
    const { asset, ...wrappedComponentProps } = funcProps

    // state
    const [imageLoadFailed, setImageLoadFailed] = React.useState(false)

    const isNative = asset && isNativeAsset(asset)

    // Reset imageLoadFailed when the asset changes
    React.useEffect(() => {
      setImageLoadFailed(false)
    }, [asset?.logo, asset?.contractAddress])

    const nativeAssetLogo =
      isNative && asset ? makeNativeAssetLogo(asset.symbol, asset.chainId) : ''

    const initialTokenImageURL = nativeAssetLogo || asset?.logo || ''

    const isRemoteURL = isRemoteImageURL(initialTokenImageURL)

    const isNonFungibleToken = asset?.isNft || asset?.isErc721

    const tokenImageURL =
      isRemoteURL || isNonFungibleToken || isNative || isStorybook
        ? initialTokenImageURL
        : `chrome://erc-token-images/${encodeURIComponent(
            initialTokenImageURL,
          )}`

    // memos + computed
    const isValidIcon = React.useMemo(() => {
      if (isStorybook) {
        return !!asset?.logo
      }

      const isDataUri = isDataURL(tokenImageURL)

      if (isRemoteURL || isDataUri) {
        return tokenImageURL?.includes('data:image/') || isNonFungibleToken
          ? true
          : isValidIconExtension(new URL(tokenImageURL).pathname)
      }
      return false
    }, [asset?.logo, isRemoteURL, tokenImageURL, isNonFungibleToken])

    const needsPlaceholder =
      (tokenImageURL === '' || !isValidIcon || imageLoadFailed)
      && nativeAssetLogo === ''

    const bg = React.useMemo(() => {
      if (needsPlaceholder) {
        return background({
          seed: asset?.contractAddress
            ? asset?.contractAddress.toLowerCase()
            : asset?.name,
        })
      }
    }, [needsPlaceholder, asset?.contractAddress, asset?.name])

    const remoteImage = React.useMemo(() => {
      // If the token is a non-fungible token and the wrapped component is
      // NftIcon, return the token image URL.
      // chrome-untrusted://image?url=... will be applied in nft_script.ts.
      if (isNonFungibleToken && WrappedComponent.name === 'NftIcon') {
        return tokenImageURL
      }
      if (isRemoteURL) {
        return `chrome://image?url=${encodeURIComponent(
          tokenImageURL,
        )}&staticEncode=true`
      }
      return ''
    }, [isRemoteURL, tokenImageURL, isNonFungibleToken])

    // render
    if (!asset) {
      return null
    }

    const icon = nativeAssetLogo || (isRemoteURL ? remoteImage : tokenImageURL)

    if (needsPlaceholder && isNonFungibleToken) {
      return (
        <NFTPlaceholderWrapper
          marginLeft={marginLeft ?? 0}
          marginRight={marginRight ?? 0}
          size={size}
        >
          <NFTPlacholderIcon />
        </NFTPlaceholderWrapper>
      )
    }

    if (needsPlaceholder || !icon) {
      return (
        <IconWrapper
          panelBackground={bg}
          isPlaceholder={true}
          size={size}
          marginLeft={marginLeft ?? 0}
          marginRight={marginRight ?? 0}
        >
          <PlaceholderText size={size}>
            {asset?.symbol.charAt(0) || '?'}
          </PlaceholderText>
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
          {...(wrappedComponentProps as PROPS_FOR_FUNCTION & {
            icon?: undefined
          })}
          icon={isStorybook ? tokenImageURL : icon}
          onError={() => setImageLoadFailed(true)}
        />
      </IconWrapper>
    )
  }
}

export default withPlaceholderIcon
