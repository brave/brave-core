// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

/* eslint-disable @typescript-eslint/key-spacing */

import * as React from 'react'
import { background } from 'ethereum-blockies'
import { skipToken } from '@reduxjs/toolkit/query'

// Constants
import { BraveWallet } from '../../../constants/types'

// Utils
import {
  stripERC20TokenImageURL,
  isRemoteImageURL,
  isValidIconExtension,
  isDataURL,
  isIpfs,
  isComponentInStorybook,
  stripChromeImageURL
} from '../../../utils/string-utils'

// Hooks
import {
  useGetIpfsGatewayTranslatedNftUrlQuery //
} from '../../../common/slices/api.slice'

// Styled components
import { IconWrapper, PlaceholderText } from './style'

// Options
import { makeNativeAssetLogo } from '../../../options/asset-options'

interface Config {
  size: 'big' | 'medium' | 'small' | 'tiny'
  marginLeft?: number
  marginRight?: number
}

interface Props {
  asset:
    | Pick<
        BraveWallet.BlockchainToken,
        'symbol' | 'logo' | 'isNft' | 'isErc721' | 'contractAddress' | 'name'
      >
    | undefined
  network: Pick<BraveWallet.NetworkInfo, 'chainId' | 'symbol'> | undefined
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
    const { asset, network, ...wrappedComponentProps } = funcProps

    const isNativeAsset =
      asset &&
      network &&
      asset.symbol.toLowerCase() === network.symbol.toLowerCase()

    const nativeAssetLogo = isNativeAsset
      ? makeNativeAssetLogo(network.symbol, network.chainId)
      : null

    const tokenImageURL = stripERC20TokenImageURL(
      nativeAssetLogo || asset?.logo || ''
    )
    const isRemoteURL = isRemoteImageURL(tokenImageURL)

    const isNonFungibleToken = asset?.isNft || asset?.isErc721

    // queries
    const { data: ipfsUrl } = useGetIpfsGatewayTranslatedNftUrlQuery(
      tokenImageURL || skipToken
    )

    // memos + computed
    const isValidIcon = React.useMemo(() => {
      if (isStorybook) {
        return !!asset?.logo
      }

      const isDataUri = isDataURL(asset?.logo)

      if (isRemoteURL || isDataUri) {
        return tokenImageURL?.includes('data:image/') ||
          isIpfs(tokenImageURL) ||
          isNonFungibleToken
          ? true
          : isValidIconExtension(new URL(asset?.logo || '').pathname)
      }
      return false
    }, [isRemoteURL, tokenImageURL, asset?.logo, isStorybook])

    const needsPlaceholder = isNativeAsset
      ? (tokenImageURL === '' || !isValidIcon) && nativeAssetLogo === ''
      : tokenImageURL === '' || !isValidIcon

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
        return isStorybook ? ipfsUrl || '' : `chrome://image?${ipfsUrl}`
      }
      return ''
    }, [isRemoteURL, tokenImageURL, ipfsUrl])

    // render
    if (!asset || !network) {
      return null
    }

    if (needsPlaceholder) {
      return (
        <IconWrapper
          panelBackground={bg}
          isPlaceholder={true}
          size={size}
          marginLeft={marginLeft ?? 0}
          marginRight={marginRight ?? 0}
        >
          <PlaceholderText size={size}>
            {asset.symbol.charAt(0)}
          </PlaceholderText>
        </IconWrapper>
      )
    }

    const icon = isNativeAsset && nativeAssetLogo
      ? nativeAssetLogo
      : isRemoteURL
        ? remoteImage
        : asset.logo

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
