// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// components
import { ImageLoader } from './image-loader'

// styles
import { ImageWrapper } from './nft-image.styles'
import {
  NftUiCommand,
  UpdateNftImageLoadingMessage,
  sendMessageToWalletUi
} from '../../nft-ui-messages'

interface Props {
  imageUrl: string
  mimeType: string
}
export const NftImage = (props: Props) => {
  const { imageUrl } = props

  // methods
  const onImageLoaded = React.useCallback(() => {
    const message: UpdateNftImageLoadingMessage = {
      command: NftUiCommand.UpdateNftImageLoading,
      payload: false
    }
    sendMessageToWalletUi(parent, message)
  }, [])

  return (
    <ImageWrapper>
      <ImageLoader
        src={imageUrl}
        onLoaded={onImageLoaded}
        width='auto'
        height='360px'
      />
    </ImageWrapper>
  )
}
