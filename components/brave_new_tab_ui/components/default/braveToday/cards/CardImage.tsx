// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as Card from '../cardSizes'
import * as Background from '../../../../../common/Background'

type Props = {
  imageUrl: string
  list?: boolean
  isPromoted?: boolean
  onLoaded?: () => any
}

function useGetUnpaddedImage (paddedUrl: string, onLoaded?: () => any) {
  const [unpaddedUrl, setUnpaddedUrl] = React.useState('')
  const onReceiveUnpaddedUrl = (result: string) => {
    setUnpaddedUrl(result)
    window.requestAnimationFrame(() => {
      if (onLoaded) {
        onLoaded()
      }
    })
  }
  React.useEffect(() => {
    // Storybook method
    // @ts-ignore
    if (window.braveStorybookUnpadUrl) {
      // @ts-ignore
      window.braveStorybookUnpadUrl(paddedUrl)
      .then(onReceiveUnpaddedUrl)
      return
    }
    Background.send<BraveToday.Messages.GetImageDataResponse,
        BraveToday.Messages.GetImageDataPayload>(
      Background.MessageTypes.Today.getImageData,
      { url: paddedUrl }
    )
    .then(result => onReceiveUnpaddedUrl(result.dataUrl))
    .catch(err => {
      console.error(`Error getting image for ${paddedUrl}.`, err)
    })

  }, [paddedUrl])
  return unpaddedUrl
}

export default function CardImage (props: Props) {
  const unpaddedUrl = useGetUnpaddedImage(props.imageUrl, props.onLoaded)
  const [isImageLoaded, setIsImageLoaded] = React.useState(false)
  React.useEffect(() => {
    if (unpaddedUrl) {
      const img = new Image()
      img.addEventListener('load', () => {
        setIsImageLoaded(true)
      })
      img.src = unpaddedUrl
    }
  }, [unpaddedUrl])
  const Frame = props.list ? Card.ListImageFrame : Card.ImageFrame
  return (
    <Frame isImageLoaded={isImageLoaded}>
      <Card.Image isPromoted={props.isPromoted} src={unpaddedUrl} />
    </Frame>
  )
}
