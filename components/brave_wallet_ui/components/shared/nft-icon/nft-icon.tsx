// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { CSSProperties } from 'react'

// utils
import {
  NftUiCommand,
  sendMessageToNftUiFrame,
  UpdateLoadingMessage,
  UpdateNFtMetadataMessage
} from '../../../nft/nft-ui-messages'
import { addIpfsGateway, stripERC20TokenImageURL } from '../../../utils/string-utils'

// styles
import {
  NftImageIframe,
  NftImageResponsiveIframe
} from './nft-icon-styles'

export interface NftIconProps {
  icon?: string
  responsive?: boolean
  iconStyles?: CSSProperties
  circular?: boolean
  onLoad?: () => void
}

export const NftIcon = (props: NftIconProps) => {
  const { icon, responsive, iconStyles, circular, onLoad } = props
  const [loaded, setLoaded] = React.useState<boolean>()
  const nftImageIframeRef = React.useRef<HTMLIFrameElement>(null)

  const tokenImageURL = stripERC20TokenImageURL(icon)

  const remoteImage = React.useMemo(() => {
    return addIpfsGateway(tokenImageURL)
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
          icon: remoteImage
        }
      }
      sendMessageToNftUiFrame(nftImageIframeRef.current.contentWindow, command)
      sendMessageToNftUiFrame(nftImageIframeRef.current.contentWindow, loadingCommand)
    }
  }, [loaded, remoteImage, nftImageIframeRef])

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
        circular={circular}
      />
      : <NftImageIframe
        style={iconStyles}
        onLoad={onIframeLoaded}
        ref={nftImageIframeRef}
        src="chrome-untrusted://nft-display"
        sandbox="allow-scripts allow-same-origin"
        circular={circular}
      />
  )
}
