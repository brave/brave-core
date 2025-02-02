/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled from 'styled-components'
import { loadTimeData } from '$web-common/loadTimeData'

export interface SponsoredRichMediaBackgroundInfo {
  url: string
  creativeInstanceId: string
  placementId: string
  targetUrl: string
}

type SponsoredRichMediaEventType = 'click' | 'interaction' | 'mediaPlay' | 'media25' | 'media50' | 'media75' | 'media100'

interface StatusProps {
  richMediaHasLoaded: boolean
}

interface Props extends StatusProps {
  sponsoredRichMediaBackgroundInfo: SponsoredRichMediaBackgroundInfo
  onEventReported: (name: SponsoredRichMediaEventType) => void
  onLoaded: () => void
}

const iframeAllow = `
  accelerometer 'none';
  ambient-light-sensor 'none';
  camera 'none';
  display-capture 'none';
  document-domain 'none';
  fullscreen 'none';
  geolocation 'none';
  gyroscope 'none';
  magnetometer 'none';
  microphone 'none';
  midi 'none';
  payment 'none';
  publickey-credentials-get 'none';
  usb 'none'
`.trim().replace(/\n/g, '')

const SponsoredRichMediaBackgroundIframe = styled('iframe') <StatusProps>`
  opacity: ${p => p.richMediaHasLoaded ? 1 : 0};
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  border: none;
  z-index: 0;
`

/// We expect the event data to be of the following format:
/// {
///   type: 'richMediaEvent',
///   value: 'click'
/// }
function getEventType(event: MessageEvent): SponsoredRichMediaEventType | undefined {
  if (!event.data || event.data.type !== 'richMediaEvent') {
    return undefined
  }

  if (event.data.value === 'click' || event.data.value === 'interaction'
    || event.data.value === 'mediaPlay' || event.data.value === 'media25'
    || event.data.value === 'media50' || event.data.value === 'media75'
    || event.data.value === 'media100') {
    return event.data.value
  }

  return undefined
}

export function SponsoredRichMediaBackground(props: Props) {
  const iframeRef = React.useRef<HTMLIFrameElement | null>(null)
  const { sponsoredRichMediaBackgroundInfo } = props

  React.useEffect(() => {
    try {
      const ntpSponsoredRichMediaUrlOrigin =
        new URL(loadTimeData.getString('ntpSponsoredRichMediaUrl')).origin

      const listener = (event: MessageEvent) => {
        if (!iframeRef.current) {
          return
        }

        if (!event.origin || event.origin !== ntpSponsoredRichMediaUrlOrigin) {
          return
        }

        const { contentWindow } = iframeRef.current
        if (!event.source || event.source !== contentWindow || !event.data) {
          return
        }

        const eventType = getEventType(event)
        if (!eventType) {
          return
        }

        props.onEventReported(eventType)
      }

      window.addEventListener('message', listener)
      return () => { window.removeEventListener('message', listener) }
    } catch (e) {
      console.error('Error setting up sponsored rich media event listener')
      return () => { }
    }
  }, [props.onEventReported])

  return (
    <SponsoredRichMediaBackgroundIframe
      ref={iframeRef}
      richMediaHasLoaded={props.richMediaHasLoaded}
      allow={iframeAllow}
      src={sponsoredRichMediaBackgroundInfo.url}
      sandbox='allow-scripts allow-same-origin'
      onLoad={props.onLoaded}>
    </SponsoredRichMediaBackgroundIframe>
  )
}
