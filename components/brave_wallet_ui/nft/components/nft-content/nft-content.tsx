// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import { NFTMetadataReturnType } from '../../../constants/types'
import { DisplayMode } from '../../nft-ui-messages'

// components
import Placeholder from '../../../assets/svg-icons/nft-placeholder.svg'
import { NftMultimedia } from '../nft-multimedia/nft-multimedia'
import { stripChromeImageURL } from '../../../utils/string-utils'
import { ImageWrapper } from './nft-content-styles'
import { ImageLoader } from '../nft-image/image-loader'

interface Props {
  isLoading?: boolean
  displayMode?: DisplayMode
  nftMetadata?: NFTMetadataReturnType
  imageUrl?: string
}

export const NftContent = (props: Props) => {
  const { nftMetadata, imageUrl, displayMode } = props
  const wrapperRef = React.useRef<HTMLDivElement>(null)
  const [isInView, setIsInView] = React.useState<boolean>(false)

  const url = React.useMemo(() => {
    return stripChromeImageURL(imageUrl) ?? Placeholder
  }, [imageUrl])

  const onIntersection = React.useCallback(
    (entries: IntersectionObserverEntry[]) => {
      const [entry] = entries
      if (entry.isIntersecting) {
        setIsInView(true)
      }
    },
    []
  )

  React.useEffect(() => {
    if (!wrapperRef?.current) return
    const observer = new IntersectionObserver(onIntersection, {})
    observer.observe(wrapperRef.current)

    // Clean up the observer when the component unmounts
    return () => observer.disconnect()
  }, [wrapperRef])

  return (
    <>
      <div ref={wrapperRef}>
        {url && displayMode === 'icon' && isInView ? (
          <ImageWrapper>
            <ImageLoader src={url} />
          </ImageWrapper>
        ) : null}
      </div>
      {displayMode === 'details' && nftMetadata ? (
        <NftMultimedia nftMetadata={nftMetadata} />
      ) : null}
    </>
  )
}
