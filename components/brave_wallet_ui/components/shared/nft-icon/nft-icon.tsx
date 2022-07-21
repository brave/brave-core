import * as React from 'react'

import { AssetIconProps } from '../style'
import {
  NftUiCommand,
  sendMessageToNftUiFrame,
  UpdateNftImageUrl
} from '../../../nft/nft-ui-messages'

import { NftImageIframe } from './nft-icon-styles'

export const NftIcon = ({ icon }: AssetIconProps) => {
  const [loaded, setLoaded] = React.useState<boolean>()
  const nftImageIframeRef = React.useRef<HTMLIFrameElement>(null)

  React.useEffect(() => {
    if (loaded && icon && nftImageIframeRef && nftImageIframeRef.current) {
      const command: UpdateNftImageUrl = {
        command: NftUiCommand.UpdateNFTImageUrl,
        payload: icon
      }
      sendMessageToNftUiFrame(nftImageIframeRef.current.contentWindow, command)
    }
  }, [loaded, icon, nftImageIframeRef])

  return (
    <NftImageIframe
      onLoad={() => setLoaded(true)}
      ref={nftImageIframeRef}
      src="chrome-untrusted://nft-display"
      sandbox="allow-scripts allow-same-origin"
    />
  )
}
