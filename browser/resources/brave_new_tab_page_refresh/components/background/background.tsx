/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { NewTabPageAdEventType, SponsoredImageBackground } from '../../state/background_state'
import { openLink } from '../common/link'
import { loadImage } from '../../lib/image_loader'
import { useWidgetLayoutReady } from '../app_layout_ready'
import { debounce } from '$web-common/debounce'
import { useSearchMatches, useSearchActions } from '../../context/search_context'

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
            className='sponsored'
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

interface ImageBackgroundProps {
  url: string
  className?: string
  onLoadError?: () => void
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

  return <div className={classNames.join(' ')} />
}

const richMediaSearchBoxKey = 'rich-media-search-box'

function SponsoredRichMediaBackground(
  props: { background: SponsoredImageBackground }
) {
  const actions = useBackgroundActions()
  const sponsoredRichMediaBaseUrl =
    useBackgroundState((s) => s.sponsoredRichMediaBaseUrl)
  const [frameHandle, setFrameHandle] = React.useState<IframeBackgroundHandle>()

  useSafeAreaReporter(frameHandle)

  // TODO(https://github.com/brave/brave-browser/issues/49471): [NTP Next]
  // Refactor rich media background components.
  const searchActions = useSearchActions()
  useSearchMatchesReporter(frameHandle)

  return (
    <IframeBackground
      url={props.background.imageUrl}
      expectedOrigin={new URL(sponsoredRichMediaBaseUrl).origin}
      onReady={setFrameHandle}
      onMessage={(data: any) => {
        if (!data ||
          typeof data !== 'object' ||
          typeof data.type !== 'string') {
          return
        }

        switch (data.type) {
          case 'richMediaEvent': {
            const value = String(data.value ?? '')
            const eventType = getRichMediaEventType(value)
            if (eventType) {
              actions.notifySponsoredRichMediaEvent(eventType)
              if (eventType === NewTabPageAdEventType.kClicked) {
                const url = props.background.logo?.destinationUrl
                if (url) {
                  openLink(url)
                }
              }
            }
            break
          }
          case 'richMediaQueryBraveSearchAutocomplete': {
            const value = String(data.value ?? '')
            if (value) {
              searchActions.setActiveSearchInputKey(richMediaSearchBoxKey)
              searchActions.queryAutocomplete(value, 'search.brave.com')
            }
            break
          }
          case 'richMediaOpenBraveSearchWithQuery': {
            const value = String(data.value ?? '')
            if (value) {
              // Opening Brave Search from rich media is treated as an ad click
              // for reporting purposes.
              actions.notifySponsoredRichMediaEvent(
                NewTabPageAdEventType.kClicked)
              openLink(`https://search.brave.com/${value}`)
            }
            break
          }
          default:
            break
        }
      }}
    />
  )
}

// Posts a message to the rich media background iframe containing a rectangle
// that is empty of content and can be used to display interactive elements.
function useSafeAreaReporter(frameHandle?: IframeBackgroundHandle) {
  const widgetLayoutReady = useWidgetLayoutReady()

  React.useEffect(() => {
    if (!widgetLayoutReady || !frameHandle) {
      return
    }

    const selector = '.sponsored-background-safe-area'
    const safeArea = document.querySelector<HTMLDivElement>(selector)
    if (!safeArea) {
      return
    }

    const postSafeArea = debounce(() => {
      if (!safeArea) {
        return
      }
      const rect = safeArea.getBoundingClientRect()
      frameHandle.postMessage({
        type: 'richMediaSafeRect',
        value: {
          x: rect.x + window.scrollX,
          y: rect.y + window.scrollY,
          width: rect.width,
          height: rect.height
        }
      })
    }, 120)

    postSafeArea()

    const resizeObserver = new ResizeObserver(postSafeArea)
    resizeObserver.observe(safeArea)
    return () => { resizeObserver.disconnect() }
  }, [widgetLayoutReady, frameHandle])
}

function getRichMediaEventType(value: string): NewTabPageAdEventType | null {
  switch (value) {
    case 'click': return NewTabPageAdEventType.kClicked
    case 'interaction': return NewTabPageAdEventType.kInteraction
    case 'mediaPlay': return NewTabPageAdEventType.kMediaPlay
    case 'media25': return NewTabPageAdEventType.kMedia25
    case 'media100': return NewTabPageAdEventType.kMedia100
    default: return null
  }
}

// Posts a message to the rich media background iframe containing the current
// list of search matches.
function useSearchMatchesReporter(frameHandle?: IframeBackgroundHandle) {
  const searchMatches = useSearchMatches(richMediaSearchBoxKey)

  React.useEffect(() => {
    if (!frameHandle || !searchMatches) {
      return
    }

    const postSearchMatches = debounce(() => {
      frameHandle.postMessage({
        type: 'richMediaSearchMatches',
        value: searchMatches
      })
    }, 120)

    postSearchMatches()
  }, [frameHandle, searchMatches])
}

interface IframeBackgroundHandle {
  postMessage: (data: unknown) => void
}

interface IframeBackgroundProps {
  url: string
  expectedOrigin: string
  onReady: (handle: IframeBackgroundHandle) => void
  onMessage: (data: unknown) => void
}

function IframeBackground(props: IframeBackgroundProps) {
  const iframeRef = React.useRef<HTMLIFrameElement>(null)
  const [contentLoaded, setContentLoaded] = React.useState(false)

  React.useEffect(() => {
    function listener(event: MessageEvent) {
      if (!event.origin || event.origin !== props.expectedOrigin) {
        return
      }
      if (!event.source || event.source !== iframeRef.current?.contentWindow) {
        return
      }
      props.onMessage(event.data)
    }

    window.addEventListener('message', listener)
    return () => { window.removeEventListener('message', listener) }
  }, [props.expectedOrigin, props.onMessage])

  React.useEffect(() => {
    if (!props.onReady || !contentLoaded) {
      return
    }
    props.onReady({
      postMessage: (data) => {
        const window = iframeRef.current?.contentWindow
        window?.postMessage(data, props.expectedOrigin)
      }
    })
  }, [props.onReady, props.expectedOrigin, contentLoaded])

  return (
    <iframe
      ref={iframeRef}
      className={contentLoaded ? '' : 'loading'}
      src={props.url}
      sandbox='allow-scripts allow-same-origin'
      allow={allowNoneList([
        'accelerometer',
        'ambient-light-sensor',
        'camera',
        'display-capture',
        'document-domain',
        'fullscreen',
        'geolocation',
        'gyroscope',
        'magnetometer',
        'microphone',
        'midi',
        'payment',
        'publickey-credentials-get',
        'usb'
      ])}
      onLoad={() => setContentLoaded(true)}
    />
  )
}

function allowNoneList(items: string[]) {
  return items.map((key) => `${key} 'none'`).join('; ')
}
