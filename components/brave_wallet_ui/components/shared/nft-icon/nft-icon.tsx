// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { CSSProperties } from 'react'
import { skipToken } from '@reduxjs/toolkit/query'

// utils
import {
  NftUiCommand,
  sendMessageToNftUiFrame,
  UpdateLoadingMessage,
  UpdateNFtMetadataMessage
} from '../../../nft/nft-ui-messages'
import {
  isComponentInStorybook,
  stripERC20TokenImageURL
} from '../../../utils/string-utils'

// hooks
import {
  useGetIpfsGatewayTranslatedNftUrlQuery,
  useGetIPFSUrlFromGatewayLikeUrlQuery
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
  onLoad?: () => void
}

const isStorybook = isComponentInStorybook()

const loadingCommand: UpdateLoadingMessage = {
  command: NftUiCommand.UpdateLoading,
  payload: false
}

export const NftIcon = (props: NftIconProps) => {
  const { icon, responsive, iconStyles, onLoad } = props
  const tokenImageURL = icon ? stripERC20TokenImageURL(icon) : ''

  // refs
  const nftImageIframeRef = React.useRef<HTMLIFrameElement>(null)

  // state
  const [loaded, setLoaded] = React.useState<boolean>()

  // queries
  const { data: remoteImage } = useGetIpfsGatewayTranslatedNftUrlQuery(
    tokenImageURL || skipToken
  )
  const { data: remoteCid } = useGetIPFSUrlFromGatewayLikeUrlQuery(
    tokenImageURL || skipToken
  )

  // methods
  const onIframeLoaded = React.useCallback(() => {
    setLoaded(true)
    if (onLoad) {
      onLoad()
    }
  }, [onLoad])

  React.useEffect(() => {
    if (!loaded && isStorybook) {
      setLoaded(true)
      return
    }

    if (loaded && icon !== undefined && nftImageIframeRef?.current) {
      const command: UpdateNFtMetadataMessage = {
        command: NftUiCommand.UpdateNFTMetadata,
        payload: {
          displayMode: 'icon',
          icon: remoteImage || undefined,
          imageCID: remoteCid || undefined
        }
      }
      sendMessageToNftUiFrame(nftImageIframeRef.current.contentWindow, command)
      sendMessageToNftUiFrame(
        nftImageIframeRef.current.contentWindow,
        loadingCommand
      )
    }
  }, [loaded, remoteImage, nftImageIframeRef, remoteCid])

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
      style={iconStyles}
      onLoad={onIframeLoaded}
      ref={nftImageIframeRef}
      src='chrome-untrusted://nft-display'
      sandbox='allow-scripts allow-same-origin'
    />
  ) : (
    <NftImageIframe
      style={iconStyles}
      onLoad={onIframeLoaded}
      ref={nftImageIframeRef}
      src='chrome-untrusted://nft-display'
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
