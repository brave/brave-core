// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as Card from '../cardSizes'
import * as BraveNews from '../../../../../brave_news/browser/resources/shared/api'
import { useUnpaddedImageUrl } from '../../../../../brave_news/browser/resources/shared/useUnpaddedImageUrl'

type Props = {
  imageUrl?: string
  list?: boolean
  isPromoted?: boolean
  onLoaded?: () => any
}

export default function CardImage(props: Props) {
  const unpaddedUrl = useUnpaddedImageUrl(props.imageUrl, props.onLoaded)
  const [isImageLoaded, setIsImageLoaded] = React.useState(false)
  React.useEffect(() => {
    if (unpaddedUrl) {
      const img = new Image()
      let shouldCancel = false
      img.addEventListener('load', () => {
        if (!shouldCancel) {
          setIsImageLoaded(true)
        }
      })
      img.src = unpaddedUrl
      return () => { shouldCancel = true }
    }

    return () => { }
  }, [unpaddedUrl])
  const Frame = props.list ? Card.ListImageFrame : Card.ImageFrame
  return (
    <Frame data-source={props.imageUrl} isImageLoaded={isImageLoaded}>
      <Card.Image isPromoted={props.isPromoted} src={unpaddedUrl} />
    </Frame>
  )
}

type FromFeedItemProps = Omit<Props, 'imageUrl' | 'isUnpadded'> & {
  data: BraveNews.FeedItemMetadata
}

export function CardImageFromFeedItem(props: FromFeedItemProps) {
  React.useEffect(() => {
    if (!props.data.image.imageUrl && !props.data.image.paddedImageUrl) {
      // Shouldn't happen since backend filters out items
      // with no image.
      // It can happen for direct-feed items, and we just don't display the
      // image.
      // This is in a useEffect so it does not log every render.
      console.warn('Brave News found item with no image', props.data.url.url)
    }
  }, [props.data.image.imageUrl, props.data.image.paddedImageUrl])
  const imageUrl = React.useMemo(() => {
    if (props.data.image.imageUrl?.url) { return props.data.image.imageUrl.url }
    if (props.data.image.paddedImageUrl?.url) { return props.data.image.paddedImageUrl.url }
    return ''
  }, [props.data.image.imageUrl, props.data.image.paddedImageUrl, props.data.description])
  const { data, ...baseProps } = props
  if (imageUrl) {
    return (
      <CardImage
        {...baseProps}
        imageUrl={imageUrl}
        isPromoted={props.isPromoted}
      />
    )
  }
  return null
}
