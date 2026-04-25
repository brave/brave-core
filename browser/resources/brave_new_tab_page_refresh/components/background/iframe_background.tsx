/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

export interface IframeBackgroundHandle {
  postMessage: (data: unknown) => void
}

interface Props {
  url: string
  expectedOrigin: string
  onReady: (handle: IframeBackgroundHandle) => void
  onMessage: (data: unknown) => void
}

export function IframeBackground(props: Props) {
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
    return () => {
      window.removeEventListener('message', listener)
    }
  }, [props.expectedOrigin, props.onMessage])

  React.useEffect(() => {
    if (!props.onReady || !contentLoaded) {
      return
    }
    props.onReady({
      postMessage: (data) => {
        const window = iframeRef.current?.contentWindow
        window?.postMessage(data, props.expectedOrigin)
      },
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
        'usb',
      ])}
      onLoad={() => setContentLoaded(true)}
    />
  )
}

function allowNoneList(items: string[]) {
  return items.map((key) => `${key} 'none'`).join('; ')
}
