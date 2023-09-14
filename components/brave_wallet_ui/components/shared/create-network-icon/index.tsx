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

type SimpleNetwork = Pick<
  BraveWallet.NetworkInfo,
  'iconUrls' | 'chainId' | 'symbol' | 'chainName'
>

interface Props {
  network?: SimpleNetwork
  marginRight?: number
  size?: 'huge' | 'big' | 'small' | 'tiny' | 'extra-small'
}

const isStorybook = isComponentInStorybook()

export const CreateNetworkIcon = ({ network, marginRight, size }: Props) => {
  // exit early if no network
  if (!network) {
    return (
      <NetworkPlaceholderIcon marginRight={marginRight} network={network} />
    )
  }

  // computed
  const networkLogo = getNetworkLogo(network.chainId, network.symbol)
  const isTestnet = SupportedTestNetworks.includes(network.chainId)

  // simple render
  if (networkLogo) {
    return (
      <IconWrapper
        marginRight={marginRight ?? 0}
        isTestnet={isTestnet}
        size={size}
      >
        <NetworkIcon size={size} icon={networkLogo} />
      </IconWrapper>
    )
  }

  // complex compute + render
  const networkIcon = network.iconUrls[0]
  const isSandboxUrl = networkIcon?.startsWith('chrome://erc-token-images/')
  const networkImageURL = isSandboxUrl
    ? stripERC20TokenImageURL(networkIcon)
    : networkIcon

  const isRemoteURL = isRemoteImageURL(networkImageURL)

  // needs placeholder
  if (
    !networkImageURL ||
    !isStorybook ||
    (isRemoteURL || isSandboxUrl
      ? !isValidIconExtension(new URL(networkIcon).pathname)
      : true)
  ) {
    return (
      <NetworkPlaceholderIcon marginRight={marginRight} network={network} />
    )
  }

  return (
    <IconWrapper
      marginRight={marginRight ?? 0}
      isTestnet={isTestnet}
      size={size}
    >
      <NetworkIcon
        size={size}
        icon={isRemoteURL ? `chrome://image?${networkImageURL}` : networkIcon}
      />
    </IconWrapper>
  )
}

export default CreateNetworkIcon

function NetworkPlaceholderIcon({
  marginRight,
  network
}: {
  marginRight: number | undefined
  network?: SimpleNetwork
}) {
  // custom hooks
  const orb = useNetworkOrb(network)

  // render
  return (
    <IconWrapper marginRight={marginRight ?? 0} isTestnet={false}>
      <Placeholder orb={orb} />
    </IconWrapper>
  )
}
