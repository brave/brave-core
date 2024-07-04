// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { CSSProperties } from 'react'
import { skipToken } from '@reduxjs/toolkit/query'

// utils
import {
  isComponentInStorybook,
  stripERC20TokenImageURL
} from '../../../utils/string-utils'

// hooks
import {
  useGetIpfsGatewayTranslatedNftUrlQuery //
} from '../../../common/slices/api.slice'

// styles
import {
  NftImageIframe,
  NftImageResponsiveIframe,
  NftPlaceholderWrapper,
  NFTPlacholderIcon
} from './nft-icon-styles'

export interface NftIconProps {
  icon?: string | null
  responsive?: boolean
  iconStyles?: CSSProperties
}

const isStorybook = isComponentInStorybook()

export const NftIcon = (props: NftIconProps) => {
  const { icon, responsive, iconStyles } = props
  const tokenImageURL = icon ? stripERC20TokenImageURL(icon) : ''

  // refs
  const nftImageIframeRef = React.useRef<HTMLIFrameElement>(null)

  // queries
  const { data: remoteImage } = useGetIpfsGatewayTranslatedNftUrlQuery(
    tokenImageURL || skipToken
  )

  // memos
  const nftIframeUrl = React.useMemo(() => {
    const urlParams = new URLSearchParams({
      displayMode: 'icon',
      icon: encodeURI(remoteImage || '')
    })
    return `chrome-untrusted://nft-display?${urlParams.toString()}`
  }, [remoteImage])

  // computed
  /** iframe should re-render when url changes */
  const iframeKey = `${responsive ? 'responsive' : 'fixed'}-${nftIframeUrl}`

  // render
  if (isStorybook) {
    return <StorybookNftIcon {...props} />
  }

  if (tokenImageURL === '') {
    return (
      <NftPlaceholderWrapper>
        <NFTPlacholderIcon />
      </NftPlaceholderWrapper>
    )
  }

  return responsive ? (
    <NftImageResponsiveIframe
      key={iframeKey}
      style={iconStyles}
      ref={nftImageIframeRef}
      src={nftIframeUrl}
      sandbox='allow-scripts allow-same-origin'
    />
  ) : (
    <NftImageIframe
      key={iframeKey}
      style={iconStyles}
      ref={nftImageIframeRef}
      src={nftIframeUrl}
      sandbox='allow-scripts allow-same-origin'
    />
  )
}

export const StorybookNftIcon = (props: NftIconProps) => {
  const { icon, responsive, iconStyles } = props
  const tokenImageURL = icon ? stripERC20TokenImageURL(icon) : ''

  return responsive ? (
    <NftImageResponsiveIframe
      as={'img'}
      style={iconStyles}
      src={tokenImageURL}
    />
  ) : (
    <NftImageIframe
      as={'img'}
      style={iconStyles}
      src={tokenImageURL}
    />
  )
}
