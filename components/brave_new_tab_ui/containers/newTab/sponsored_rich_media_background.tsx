/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled from 'styled-components'
import { loadTimeData } from '$web-common/loadTimeData'
import * as BraveAds from 'gen/brave/components/brave_ads/core/mojom/brave_ads.mojom.m.js'

export interface SponsoredRichMediaBackgroundInfo {
  url: string
  placementId: string
  creativeInstanceId: string
  metricType: BraveAds.NewTabPageAdMetricType
  targetUrl: string
}

// The subset of an autocomplete match posted back to the rich media
// background iframe as `richMediaSearchMatches`.
export interface RichMediaSearchMatch {
  contents: string
  description: string
  destinationUrl: string
  iconUrl: string
  imageUrl: string
  allowedToBeDefaultMatch: boolean
}

interface StatusProps {
  richMediaHasLoaded: boolean
}

interface Props extends StatusProps {
  sponsoredRichMediaBackgroundInfo: SponsoredRichMediaBackgroundInfo
  searchMatches?: RichMediaSearchMatch[]
  // Reports an ad event and, for a click, also navigates to the ad's
  // destination URL. Only appropriate for the generic `richMediaEvent`
  // message; other messages that separately trigger their own navigation
  // (e.g. opening Brave Search) must report through `onAdEventReported`
  // instead, to avoid navigating twice.
  onEventReported: (name: BraveAds.NewTabPageAdEventType) => void
  // Reports an ad event without any navigation side effect.
  onAdEventReported?: (name: BraveAds.NewTabPageAdEventType) => void
  onLoaded: () => void
  onOpenBraveSearch?: (query: string) => void
  onQueryAutocomplete?: (query: string) => void
  onMakeBraveSearchDefault?: () => void
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

  /* Blur out the content when Brave News is interacted
     with. We need the opacity to fade out our background image.
   */
  filter: blur(calc(var(--ntp-extra-content-effect-multiplier, 0) * 38px));
  opacity: max(0.3, calc(1 - var(--ntp-extra-content-effect-multiplier)));
  background: var(--default-bg-color);
`

/// We expect the event data to be of the following format:
/// {
///   type: 'richMediaEvent',
///   value: 'click'
/// }
function getEventType(value: unknown): BraveAds.NewTabPageAdEventType | undefined {
  const eventMap: { [key: string]: BraveAds.NewTabPageAdEventType } = {
    'click': BraveAds.NewTabPageAdEventType.kClicked,
    'interaction': BraveAds.NewTabPageAdEventType.kInteraction,
    'mediaPlay': BraveAds.NewTabPageAdEventType.kMediaPlay,
    'media25': BraveAds.NewTabPageAdEventType.kMedia25,
    'media100': BraveAds.NewTabPageAdEventType.kMedia100
  }

  return eventMap[value as string]
}

export interface RichMediaMessageCapabilities {
  onEventReported: (name: BraveAds.NewTabPageAdEventType) => void
  onAdEventReported?: (name: BraveAds.NewTabPageAdEventType) => void
  onOpenBraveSearch?: (query: string) => void
  onQueryAutocomplete?: (query: string) => void
  onMakeBraveSearchDefault?: () => void
}

// Reads a message posted from the rich media background iframe and executes
// the appropriate capability. Exported for testing.
export function dispatchRichMediaMessage(
  data: any,
  capabilities: RichMediaMessageCapabilities,
) {
  if (!data) {
    return
  }

  switch (data.type) {
    case 'richMediaEvent': {
      const eventType = getEventType(data.value)
      if (eventType) {
        capabilities.onEventReported(eventType)
      }
      break
    }
    case 'richMediaOpenBraveSearchWithQuery': {
      if (data.value) {
        capabilities.onAdEventReported?.(BraveAds.NewTabPageAdEventType.kClicked)
        capabilities.onOpenBraveSearch?.(String(data.value))
      }
      break
    }
    case 'richMediaQueryBraveSearchAutocomplete': {
      if (typeof data.value === 'string') {
        capabilities.onQueryAutocomplete?.(data.value)
      }
      break
    }
    case 'richMediaMakeBraveSearchDefault': {
      capabilities.onMakeBraveSearchDefault?.()
      break
    }
    case 'richMediaHideBraveSearchBox': {
      // Not supported on Android: there is no native search box visibility
      // control reachable from this WebUI. Recognized and ignored so
      // unsupported creatives don't produce a console warning.
      break
    }
  }
}

export function SponsoredRichMediaBackground(props: Props) {
  const iframeRef = React.useRef<HTMLIFrameElement | null>(null)
  const { sponsoredRichMediaBackgroundInfo } = props

  React.useEffect(() => {
    try {
      const ntpNewTabTakeoverRichMediaUrlOrigin =
        new URL(loadTimeData.getString('ntpNewTabTakeoverRichMediaUrl')).origin

      const listener = (event: MessageEvent) => {
        if (event.origin !== ntpNewTabTakeoverRichMediaUrlOrigin) {
          return
        }

        if (!iframeRef.current) {
          return
        }

        const { contentWindow } = iframeRef.current
        if (!event.source || event.source !== contentWindow || !event.data) {
          return
        }

        dispatchRichMediaMessage(event.data, {
          onEventReported: props.onEventReported,
          onAdEventReported: props.onAdEventReported,
          onOpenBraveSearch: props.onOpenBraveSearch,
          onQueryAutocomplete: props.onQueryAutocomplete,
          onMakeBraveSearchDefault: props.onMakeBraveSearchDefault,
        })
      }

      window.addEventListener('message', listener)
      return () => { window.removeEventListener('message', listener) }
    } catch (e) {
      console.error('Error setting up sponsored rich media event listener')
      return () => { }
    }
  }, [props.onEventReported, props.onAdEventReported, props.onOpenBraveSearch, props.onQueryAutocomplete, props.onMakeBraveSearchDefault])

  React.useEffect(() => {
    if (!props.searchMatches || !iframeRef.current?.contentWindow) {
      return
    }
    try {
      const ntpNewTabTakeoverRichMediaUrlOrigin =
        new URL(loadTimeData.getString('ntpNewTabTakeoverRichMediaUrl')).origin
      iframeRef.current.contentWindow.postMessage({
        type: 'richMediaSearchMatches',
        value: props.searchMatches
      }, ntpNewTabTakeoverRichMediaUrlOrigin)
    } catch (e) {
      console.error('Error posting search matches to sponsored rich media iframe')
    }
  }, [props.searchMatches])

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
