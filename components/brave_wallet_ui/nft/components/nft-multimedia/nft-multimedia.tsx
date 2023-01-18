// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// utils
import { NFTMetadataReturnType } from '../../../constants/types'
import {
  NftUiCommand,
  sendMessageToWalletUi,
  ToggleNftModal
} from '../../nft-ui-messages'
import {
  getMediaType,
  MultimediaType
} from '../../nft-utils'

// styles
import {
  MultiMediaWrapper
} from './nft-multimedia.styles'
import { NftImage } from '../nft-image/nft-image'
import placeholderImage from '../../../assets/svg-icons/nft-placeholder.svg'

const placeholderImageMimeType = 'image/svg+xml'

interface Props {
  nftMetadata: NFTMetadataReturnType
}

export const NftMultimedia = (props: Props) => {
  const {
    nftMetadata
  } = props

  // state
  const [mediaUrl, setMediaUrl] = React.useState<string>('')
  const [mimeType, setMimeType] = React.useState<string>('')
  const [mediaType, setMediaType] = React.useState<MultimediaType>()

  React.useEffect(() => {
    if (nftMetadata.animationURL && nftMetadata.animationMimeType) {
      setMediaUrl(nftMetadata.animationURL)
      setMimeType(nftMetadata.animationMimeType)
      setMediaType(getMediaType(nftMetadata.animationMimeType))
    } else if (nftMetadata.imageURL && nftMetadata.imageMimeType) {
      setMediaUrl(nftMetadata.imageURL)
      setMimeType(nftMetadata.imageMimeType)
      setMediaType(getMediaType(nftMetadata.imageMimeType))
    } else {
      setMediaUrl(placeholderImage)
      setMimeType(placeholderImageMimeType)
      setMediaType(getMediaType(placeholderImageMimeType))
    }
  }, [nftMetadata])

  const onClickMagnify = React.useCallback(() => {
    const message: ToggleNftModal = {
      command: NftUiCommand.ToggleNftModal,
      payload: true
    }
    sendMessageToWalletUi(parent, message)
    // return focus to parent window
    parent.focus()
  }, [])

  const renderMedia = React.useCallback(() => {
    if (mediaType === 'image') {
      return (
        <NftImage
          imageUrl={mediaUrl}
          mimeType={mimeType}
          onMagnify={onClickMagnify}
        />
      )
    }

    // For unsupported media type, show placeholder image
    return (
      <NftImage
        imageUrl={mediaUrl}
        mimeType={mimeType}
        onMagnify={onClickMagnify}
      />
    )
  }, [mediaUrl, mediaType])

  return (
    <MultiMediaWrapper>
      {renderMedia()}
    </MultiMediaWrapper>
  )
}
