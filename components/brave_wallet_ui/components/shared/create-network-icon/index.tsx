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
  marginRight: number
  size?: 'big' | 'small'
}

function CreateNetworkIcon (props: Props) {
  const { network, marginRight, size } = props
  if (!network) {
    return null
  }

  const nativeAsset = React.useMemo(
    () => makeNetworkAsset(network),
    [network]
  )

  const networkImageURL = stripERC20TokenImageURL(network?.iconUrls[0])
  const isRemoteURL = isRemoteImageURL(networkImageURL)
  const isDataURL = network.iconUrls[0]?.startsWith('chrome://erc-token-images/')
  const isStorybook = network.iconUrls[0]?.startsWith('static/media/components/brave_wallet_ui/')

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

  const needsPlaceholder = nativeAsset.logo === '' && (networkImageURL === '' || !isValidIcon)

  const orb = React.useMemo(() => {
    if (needsPlaceholder) {
      return create({ seed: network.chainName, size: 8, scale: 16 }).toDataURL()
    }
  }, [network])

  const remoteImage = React.useMemo(() => {
    if (isRemoteURL) {
      return `chrome://image?${networkImageURL}`
    }
    return ''
  }, [networkImageURL])

  if (needsPlaceholder) {
    return (
      <IconWrapper
        marginRight={marginRight ?? 0}
        isTestnet={false}
      >
        <Placeholder
          orb={orb}
        />
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
