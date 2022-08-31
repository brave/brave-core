// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  NftUiCommand,
  sendMessageToNftUiFrame,
  UpdateNftImageUrl,
  UpdateLoadingMessage
} from '../../../nft/nft-ui-messages'

import {
  NftImageIframe,
  NftImageResponsiveIframe
} from './nft-icon-styles'
import { CSSProperties } from 'react'

interface Props {
  icon?: string
  responsive?: boolean
  iconStyles?: CSSProperties
}

export const NftIcon = (props: Props) => {
  const { icon, responsive, iconStyles } = props
  const [loaded, setLoaded] = React.useState<boolean>()
  const nftImageIframeRef = React.useRef<HTMLIFrameElement>(null)

  const loadingCommand: UpdateLoadingMessage = {
    command: NftUiCommand.UpdateLoading,
    payload: false
  }

  React.useEffect(() => {
    if (loaded && icon && nftImageIframeRef?.current) {
      const command: UpdateNftImageUrl = {
        command: NftUiCommand.UpdateNFTImageUrl,
        payload: icon
      }
      sendMessageToNftUiFrame(nftImageIframeRef.current.contentWindow, command)
      sendMessageToNftUiFrame(nftImageIframeRef.current.contentWindow, loadingCommand)
    }
  }, [loaded, icon, nftImageIframeRef])

  return (
    responsive
      ? <NftImageResponsiveIframe
        style={iconStyles}
        onLoad={() => setLoaded(true)}
        ref={nftImageIframeRef}
        src="chrome-untrusted://nft-display"
        sandbox="allow-scripts allow-same-origin"
      />
      : <NftImageIframe
        style={iconStyles}
        onLoad={() => setLoaded(true)}
        ref={nftImageIframeRef}
        src="chrome-untrusted://nft-display"
        sandbox="allow-scripts allow-same-origin"
      />
  )
}
