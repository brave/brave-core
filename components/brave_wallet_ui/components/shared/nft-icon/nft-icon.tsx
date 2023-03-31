// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {
  extractIpfsUrl,
  translateToNftGateway
} from '../../../common/async/lib'
import * as React from 'react'
import { CSSProperties } from 'react'

// utils
import {
  NftUiCommand,
  sendMessageToNftUiFrame,
  UpdateLoadingMessage,
  UpdateNFtMetadataMessage
} from '../../../nft/nft-ui-messages'
import { stripERC20TokenImageURL } from '../../../utils/string-utils'

// styles
import {
  NftImageIframe,
  NftImageResponsiveIframe
} from './nft-icon-styles'

export interface NftIconProps {
  icon?: string
  responsive?: boolean
  iconStyles?: CSSProperties
  onLoad?: () => void
}

export const NftIcon = (props: NftIconProps) => {
  const { icon, responsive, iconStyles, onLoad } = props
  const [loaded, setLoaded] = React.useState<boolean>()
  const nftImageIframeRef = React.useRef<HTMLIFrameElement>(null)

  const tokenImageURL = stripERC20TokenImageURL(icon)
  const [remoteImage, setRemoteImage] = React.useState<string>()
  const [remoteCid, setRemoteCid] = React.useState<string>()

  React.useEffect(() => {
    let ignore = false
    translateToNftGateway(tokenImageURL).then((v) => {
      if (!ignore) setRemoteImage(v)
    })
    extractIpfsUrl(tokenImageURL).then((v) => {
      if (!ignore) setRemoteCid(v)
    })

    return () => {
      ignore = true;
    };
  }, [tokenImageURL])

  const loadingCommand: UpdateLoadingMessage = {
    command: NftUiCommand.UpdateLoading,
    payload: false
  }

  React.useEffect(() => {
    if (loaded && icon !== undefined && nftImageIframeRef?.current) {
      const command: UpdateNFtMetadataMessage = {
        command: NftUiCommand.UpdateNFTMetadata,
        payload: {
          displayMode: 'icon',
          icon: remoteImage,
          imageCID: remoteCid
        }
      }
      sendMessageToNftUiFrame(nftImageIframeRef.current.contentWindow, command)
      sendMessageToNftUiFrame(nftImageIframeRef.current.contentWindow, loadingCommand)
    }
  }, [loaded, remoteImage, nftImageIframeRef, remoteCid])

  const onIframeLoaded = React.useCallback(() => {
    setLoaded(true)
    if (onLoad) {
      onLoad()
    }
  }, [])

  return (
    responsive
      ? <NftImageResponsiveIframe
        style={iconStyles}
        onLoad={onIframeLoaded}
        ref={nftImageIframeRef}
        src="chrome-untrusted://nft-display"
        sandbox="allow-scripts allow-same-origin"
      />
      : <NftImageIframe
        style={iconStyles}
        onLoad={onIframeLoaded}
        ref={nftImageIframeRef}
        src="chrome-untrusted://nft-display"
        sandbox="allow-scripts allow-same-origin"
      />
  )
}
