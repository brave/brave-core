/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Constants
import { BraveWallet, SupportedTestNetworks } from '../../../constants/types'

// Utils
import {
  stripERC20TokenImageURL,
  isRemoteImageURL,
  isValidIconExtension,
  isComponentInStorybook
} from '../../../utils/string-utils'

// Styled components
import { IconWrapper, Placeholder, NetworkIcon } from './style'

// Options
import { getNetworkLogo } from '../../../options/asset-options'

// Hooks
import { useNetworkOrb } from '../../../common/hooks/use-orb'

interface Props {
  network?: Pick<
    BraveWallet.NetworkInfo,
    'iconUrls' | 'chainId' | 'symbol' | 'chainName'
  >
  marginRight?: number
  size?: 'huge' | 'big' | 'small' | 'tiny' | 'extra-small'
}

const isStorybook = isComponentInStorybook()

export const CreateNetworkIcon = ({
  network,
  marginRight,
  size
}: Props) => {
  // computed
  const networkImageURL = stripERC20TokenImageURL(network?.iconUrls[0])
  const isRemoteURL = isRemoteImageURL(networkImageURL)
  const isDataURL = network?.iconUrls[0]?.startsWith(
    'chrome://erc-token-images/'
  )

  const networkLogo = network
    ? getNetworkLogo(network.chainId, network.symbol)
    : ''

  const isValidIcon =
    isStorybook ||
    (network &&
      (isRemoteURL || isDataURL
        ? isValidIconExtension(new URL(network?.iconUrls[0]).pathname)
        : false))

  const needsPlaceholder =
    networkLogo === '' && (networkImageURL === '' || !isValidIcon)

  const orb = useNetworkOrb(network)

  const remoteImage = isRemoteURL
    ? `chrome://image?${networkImageURL}`
    : ''

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
      isTestnet={
        network ? SupportedTestNetworks.includes(network.chainId) : false
      }
      size={size}
    >
      <NetworkIcon
        size={size}
        icon={
          networkLogo !== ''
            ? networkLogo
            : isRemoteURL
            ? remoteImage
            : network?.iconUrls[0]
        }
      />
    </IconWrapper>
  )
}

export default CreateNetworkIcon
