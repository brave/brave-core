/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { SponsoredImageBackground } from '../../state/background_state'
import { loadImage } from '../../lib/image_loader'
import { IframeBackground, IframeBackgroundHandle } from './iframe_background'
import { useRichMediaMessageHandler } from './rich_media_message_handler'
import { useSafeAreaReporter } from './safe_area_reporter'

import {
  useBackgroundState,
  useCurrentBackground,
  useBackgroundActions } from '../../context/background_context'

import { style } from './background.style'

export function Background() {
  const actions = useBackgroundActions()
  const currentBackground = useCurrentBackground()

  function renderBackground() {
    if (!currentBackground) {
      return <ColorBackground colorValue='transparent' />
    }

    switch (currentBackground.type) {
      case 'brave':
      case 'custom':
        return <ImageBackground url={currentBackground.imageUrl} />
      case 'sponsored-image':
        return (
          <ImageBackground
            url={currentBackground.imageUrl}
            onLoadError={actions.notifySponsoredImageLoadError}
          />
        )
      case 'sponsored-rich-media':
        return <SponsoredRichMediaBackground background={currentBackground} />
      case 'color':
        return <ColorBackground colorValue={currentBackground.cssValue} />
    }
  }

  return (
    <div data-css-scope={style.scope}>
      {renderBackground()}
    </div>
  )
}

function ColorBackground(props: { colorValue: string }) {
  React.useEffect(() => {
    setBackgroundVariable(props.colorValue)
  }, [props.colorValue])

  return <div className='color-background' />
}

function setBackgroundVariable(value: string) {
  if (value) {
    document.body.style.setProperty('--ntp-background', value)
  } else {
    document.body.style.removeProperty('--ntp-background')
  }
}

function ImageBackground(props: { url: string, onLoadError?: () => void }) {
  // In order to avoid a "flash-of-unloaded-image", load the image in the
  // background and only update the background CSS variable when the image has
  // finished loading.
  React.useEffect(() => {
    loadImage(props.url).then((loaded) => {
      if (loaded) {
        setBackgroundVariable(`url(${CSS.escape(props.url)})`)
      } else if (props.onLoadError) {
        props.onLoadError()
      }
    })
  }, [props.url])

  return <div className='image-background' />
}

function SponsoredRichMediaBackground(
  props: { background: SponsoredImageBackground }
) {
  const sponsoredRichMediaBaseUrl =
    useBackgroundState((s) => s.sponsoredRichMediaBaseUrl)
  const [frameHandle, setFrameHandle] = React.useState<IframeBackgroundHandle>()
  const messageHandler = useRichMediaMessageHandler(
    props.background.logo?.destinationUrl, frameHandle)

  useSafeAreaReporter(frameHandle)

  return (
    <IframeBackground
      url={props.background.imageUrl}
      expectedOrigin={new URL(sponsoredRichMediaBaseUrl).origin}
      onReady={setFrameHandle}
      onMessage={messageHandler}
    />
  )
}
