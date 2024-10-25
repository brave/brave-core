// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

/* eslint-disable @typescript-eslint/key-spacing */

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
  isComponentInStorybook,
  stripChromeImageURL
} from '../../../utils/string-utils'
import { isNativeAsset } from '../../../utils/asset-utils'

// Styled components
import { AssetIconSizes, IconWrapper, PlaceholderText } from './style'

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
  PROPS_FOR_FUNCTION = Omit<P, 'icon' | 'onLoad'>
>(
  // ignore "onLoad" prop differences since it is not used
  WrappedComponent: React.ComponentType<
    PROPS_FOR_FUNCTION & { icon?: string | undefined }
  >,
  config: Config
): (props: Props & PROPS_FOR_FUNCTION) => JSX.Element | null {
  const { size, marginLeft, marginRight } = config

  return function (funcProps: Props & PROPS_FOR_FUNCTION) {
    const { asset, ...wrappedComponentProps } = funcProps

    const isNative = asset && isNativeAsset(asset)

    const nativeAssetLogo =
      isNative && asset ? makeNativeAssetLogo(asset.symbol, asset.chainId) : ''

    const tokenImageURL = stripERC20TokenImageURL(
      nativeAssetLogo || asset?.logo || ''
    )
    const isRemoteURL = isRemoteImageURL(tokenImageURL)

    const isNonFungibleToken = asset?.isNft || asset?.isErc721

    // memos + computed
    const isValidIcon = React.useMemo(() => {
      if (isStorybook) {
        return !!asset?.logo
      }

      const isDataUri = isDataURL(asset?.logo)

      if (isRemoteURL || isDataUri) {
        return tokenImageURL?.includes('data:image/') ||
          isNonFungibleToken
          ? true
          : isValidIconExtension(new URL(asset?.logo || '').pathname)
      }
      return false
    }, [
      asset?.logo,
      isRemoteURL,
      tokenImageURL,
      isNonFungibleToken
    ])

    const needsPlaceholder =
      (tokenImageURL === '' || !isValidIcon) && nativeAssetLogo === ''

    const bg = React.useMemo(() => {
      if (needsPlaceholder) {
        return background({
          seed: asset?.contractAddress
            ? asset?.contractAddress.toLowerCase()
            : asset?.name
        })
      }
    }, [needsPlaceholder, asset?.contractAddress, asset?.name])

    const remoteImage = React.useMemo(() => {
      if (isRemoteURL) {
        return isStorybook ? tokenImageURL || '' : `chrome://image?${tokenImageURL}`
      }
      return ''
    }, [isRemoteURL, tokenImageURL])

    // render
    if (!asset) {
      return null
    }

    const icon = nativeAssetLogo || (isRemoteURL ? remoteImage : asset?.logo)

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
          icon={isStorybook ? stripChromeImageURL(tokenImageURL) : icon}
        />
      </IconWrapper>
    )
  }
}

export default withPlaceholderIcon
