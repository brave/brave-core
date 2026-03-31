/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Dialog from '@brave/leo/react/dialog'
import Button from '@brave/leo/react/button'

import { SponsoredImageBackground } from '../../state/background_store'
import { loadImage } from '../../lib/image_loader'
import { IframeBackground, IframeBackgroundHandle } from './iframe_background'
import {
  useSafeAreaReporter,
  useRichMediaMessageHandler,
} from './rich_media_capability_provider'

import {
  useBackgroundState,
  useCurrentBackground,
  useBackgroundActions,
} from '../../context/background_context'
import { getString } from '../../lib/strings'

import { style } from './background.style'

export function Background() {
  const actions = useBackgroundActions()
  const currentBackground = useCurrentBackground()
  const [showMakeDefaultDialog, setShowMakeDefaultDialog] =
    React.useState(false)

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
            className='sponsored'
            onLoadError={actions.notifySponsoredImageLoadError}
            onContextMenu={(e) => e.preventDefault()}
          />
        )
      case 'sponsored-rich-media':
        return (
          <SponsoredRichMediaBackground
            background={currentBackground}
            onMakeBraveSearchDefault={() => setShowMakeDefaultDialog(true)}
          />
        )
      case 'color':
        return <ColorBackground colorValue={currentBackground.cssValue} />
    }
  }

  function handleMakeBraveSearchDefault() {
    actions.makeBraveSearchDefault()
    setShowMakeDefaultDialog(false)
  }

  return (
    <>
      <div data-css-scope={style.scope}>{renderBackground()}</div>
      <Dialog
        isOpen={showMakeDefaultDialog}
        showClose
        onClose={() => setShowMakeDefaultDialog(false)}
      >
        <h3>{getString(S.NEW_TAB_MAKE_SEARCH_DEFAULT_TITLE)}</h3>
        <p>{getString(S.NEW_TAB_MAKE_SEARCH_DEFAULT_TEXT)}</p>
        <div slot='actions'>
          <Button onClick={handleMakeBraveSearchDefault}>
            {getString(S.NEW_TAB_MAKE_SEARCH_DEFAULT_CONFIRM_LABEL)}
          </Button>
          <Button
            kind='outline'
            onClick={() => setShowMakeDefaultDialog(false)}
          >
            {getString(S.NEW_TAB_CANCEL_BUTTON_LABEL)}
          </Button>
        </div>
      </Dialog>
    </>
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

interface ImageBackgroundProps {
  url: string
  className?: string
  onLoadError?: () => void
  onContextMenu?: React.MouseEventHandler<HTMLDivElement>
}

function ImageBackground(props: ImageBackgroundProps) {
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

  const classNames = ['image-background']
  if (props.className) {
    classNames.push(props.className)
  }

  return (
    <div
      className={classNames.join(' ')}
      onContextMenu={props.onContextMenu}
    />
  )
}

function SponsoredRichMediaBackground(props: {
  background: SponsoredImageBackground
  onMakeBraveSearchDefault: () => void
}) {
  const sponsoredRichMediaBaseUrl = useBackgroundState(
    (s) => s.sponsoredRichMediaBaseUrl,
  )

  const [frameHandle, setFrameHandle] = React.useState<IframeBackgroundHandle>()

  const messageHandler = useRichMediaMessageHandler(frameHandle, {
    destinationUrl: props.background.logo?.destinationUrl,
    onMakeBraveSearchDefault: props.onMakeBraveSearchDefault,
  })

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
