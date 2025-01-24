/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled from 'styled-components'
import getNTPBrowserAPI from '../../api/background';

export interface RichMediaBackgroundInfo {
  url: string
  creative_instance_id: string
  placement_id: string
}

interface StatusProps {
  richMediaHasLoaded: boolean
}

interface Props extends StatusProps {
  richMediaBackgroundInfo: RichMediaBackgroundInfo
  onEventReported: (name: string) => void
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
`

const RichMediaBackgroundIframe = styled('iframe') <StatusProps>`
  --bg-opacity: ${p => p.richMediaHasLoaded ? 1 : 0};
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
///   type: 'rich_media',
///   value: 'click'
/// }
// TODO(tmancey): We are in an object called sponsored_rich_media_background.tsx so we could just have getEventType.
function getRichMediaEventType(event: MessageEvent) {
  if (!event.data || event.data.type !== 'rich_media') {
    return undefined
  }

  if (event.data.value === 'click') {
    return 'click'
  } else if (event.data.value === 'media_play') {
    return 'media_play'
  } else if (event.data.value === 'media_25') {
    return 'media_25'
  } else if (event.data.value === 'media_100') {
    return 'media_100'
  }

  return undefined
}

export function RichMediaBackground (props: Props) {
  const iframeRef = React.useRef<HTMLIFrameElement | null>(null)
  const { richMediaBackgroundInfo } = props

  React.useEffect(() => {
    const listener = (event: MessageEvent) => {
      if (!iframeRef.current) {
        return
      }

      const { contentWindow } = iframeRef.current
      if (!event.source || event.source !== contentWindow || !event.data) {
        return
      }

      const eventType = getRichMediaEventType(event)
      if (!eventType) {
        return
      }

      getNTPBrowserAPI().richMediaEventHandler.reportRichMediaEvent(
        richMediaBackgroundInfo.creative_instance_id,
        richMediaBackgroundInfo.placement_id,
        eventType)

      props.onEventReported(event.data)
    }

    window.addEventListener('message', listener)
    return () => { window.removeEventListener('message', listener) }
  }, [props.onEventReported, richMediaBackgroundInfo])

  return (
    <RichMediaBackgroundIframe
          ref={iframeRef}
          richMediaHasLoaded={props.richMediaHasLoaded}
          allow={iframeAllow.trim().replace(/\n/g, '')}
          src={richMediaBackgroundInfo.url}
          sandbox='allow-scripts allow-same-origin'>
    </RichMediaBackgroundIframe>
  )
}
