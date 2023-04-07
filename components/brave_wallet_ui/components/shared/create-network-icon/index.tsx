/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { create } from 'ethereum-blockies'

// Constants
import { BraveWallet, SupportedTestNetworks } from '../../../constants/types'

// Utils
import { stripERC20TokenImageURL, isRemoteImageURL, isValidIconExtension } from '../../../utils/string-utils'

// Styled components
import { IconWrapper, Placeholder, NetworkIcon } from './style'

// Options
import { getNetworkLogo } from '../../../options/asset-options'

interface Props {
  network?: BraveWallet.NetworkInfo
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
    return network?.iconUrls[0]?.startsWith('chrome://erc-token-images/')
  }, [network?.iconUrls[0]])

  const isStorybook = React.useMemo(() => {
    return network?.iconUrls[0]?.startsWith('static/media/components/brave_wallet_ui/')
  }, [network?.iconUrls[0]])

  const networkLogo = React.useMemo(() => {
    return network ? getNetworkLogo(network.chainId, network.symbol) : ''
  }, [network])

  const isValidIcon = React.useMemo(() => {
    if (!network) {
      return false
    }

    if (isRemoteURL || isDataURL) {
      const url = new URL(network?.iconUrls[0])
      return isValidIconExtension(url.pathname)
    }

    if (isStorybook) {
      return true
    }
    return false
  }, [isRemoteURL, isDataURL, networkImageURL])

  const needsPlaceholder = React.useMemo(() => {
    return networkLogo === '' && (networkImageURL === '' || !isValidIcon)
  }, [networkLogo, networkImageURL, isValidIcon])

  const orb = React.useMemo(() => {
    if (needsPlaceholder && network) {
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
      isTestnet={network ? SupportedTestNetworks.includes(network.chainId) : false}
    >
      <NetworkIcon
        size={size}
        icon={
          isStorybook
            ? network?.iconUrls[0]
            : networkLogo !== ''
              ? networkLogo
              : isRemoteURL ? remoteImage : network?.iconUrls[0]
        }
      />
    </IconWrapper>
  )
}

export default CreateNetworkIcon
