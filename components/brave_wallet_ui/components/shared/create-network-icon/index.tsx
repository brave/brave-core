/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { create } from 'ethereum-blockies'

// Constants
import { BraveWallet, SupportedTestNetworks } from '../../../constants/types'

// Utils
import { stripERC20TokenImageURL, isRemoteImageURL, isValidIconExtension } from '../../../utils/string-utils'

// Styled components
import { IconWrapper, Placeholder, NetworkIcon } from './style'

// Options
import { makeNetworkAsset } from '../../../options/asset-options'

interface Props {
  network: BraveWallet.NetworkInfo
  marginRight?: number
  size?: 'big' | 'small'
}

export const CreateNetworkIcon = ({
  network,
  marginRight,
  size
}: Props) => {
  // memos
  const networkImageURL = React.useMemo(() => {
    return stripERC20TokenImageURL(network?.iconUrls[0])
  }, [network?.iconUrls[0]])

  const isRemoteURL = React.useMemo(() => {
    return isRemoteImageURL(networkImageURL)
  }, [networkImageURL])

  const isDataURL = React.useMemo(() => {
    return network.iconUrls[0]?.startsWith('chrome://erc-token-images/')
  }, [network.iconUrls[0]])

  const isStorybook = React.useMemo(() => {
    return network.iconUrls[0]?.startsWith('static/media/components/brave_wallet_ui/')
  }, [network.iconUrls[0]])

  const nativeAsset = React.useMemo(() => {
    return makeNetworkAsset(network)
  }, [network])

  const isValidIcon = React.useMemo(() => {
    if (isRemoteURL || isDataURL) {
      const url = new URL(network.iconUrls[0])
      return isValidIconExtension(url.pathname)
    }

    if (isStorybook) {
      return true
    }
    return false
  }, [isRemoteURL, isDataURL, networkImageURL])

  const needsPlaceholder = React.useMemo(() => {
    return !nativeAsset || (nativeAsset?.logo === '' && (networkImageURL === '' || !isValidIcon))
  }, [nativeAsset, networkImageURL, isValidIcon])

  const orb = React.useMemo(() => {
    if (needsPlaceholder) {
      return create({ seed: network.chainName, size: 8, scale: 16 }).toDataURL()
    }
  }, [network, needsPlaceholder])

  const remoteImage = React.useMemo(() => {
    if (isRemoteURL) {
      return `chrome://image?${networkImageURL}`
    }
    return ''
  }, [networkImageURL])

  // render
  if (needsPlaceholder) {
    return (
      <IconWrapper
        marginRight={marginRight ?? 0}
        isTestnet={false}
      >
        <Placeholder orb={orb} />
      </IconWrapper>
    )
  }

  return (
    <IconWrapper
      marginRight={marginRight ?? 0}
      isTestnet={SupportedTestNetworks.includes(network.chainId)}
    >
      <NetworkIcon
        size={size}
        icon={
          nativeAsset.logo
            ? nativeAsset.logo
            : isRemoteURL ? remoteImage : network.iconUrls[0]
        }
      />
    </IconWrapper>
  )
}

export default CreateNetworkIcon
