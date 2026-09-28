/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled from 'styled-components'
import { loadTimeData } from '$web-common/loadTimeData'
import * as BraveAds from 'gen/brave/components/brave_ads/core/mojom/brave_ads.mojom.m.js'
import type { RectF } from 'gen/ui/webui/resources/tsc/mojo/ui/gfx/geometry/mojom/geometry.mojom-webui'

export interface DynamicNewTabTakeoverInfo {
  url: string
  placementId: string
  creativeInstanceId: string
  metricType: BraveAds.NewTabPageAdMetricType
  targetUrl: string
}

// The subset of an autocomplete match posted back to the dynamic-content
// background iframe as `richMediaSearchMatches`.
export interface DynamicContentSearchMatch {
  contents: string
  description: string
  destinationUrl: string
  iconUrl: string
  imageUrl: string
  allowedToBeDefaultMatch: boolean
}

interface StatusProps {
  dynamicContentHasLoaded: boolean
}

interface Props extends StatusProps {
  dynamicNewTabTakeoverInfo: DynamicNewTabTakeoverInfo
  searchMatches?: DynamicContentSearchMatch[]
  // Reports an ad event and, for a click, also navigates to the ad's
  // destination URL. Only appropriate for the generic `richMediaEvent`
  // message; other messages that separately trigger their own navigation
  // (e.g. opening Brave Search) must report through `onAdEventReported`
  // instead, to avoid navigating twice.
  onEventReported: (name: BraveAds.NewTabPageAdEventType) => void
  // Reports an ad event without any navigation side effect.
  onAdEventReported?: (name: BraveAds.NewTabPageAdEventType) => void
  onLoaded: () => void

  // A rectangle that is empty of content and can be used to display
  // interactive elements.
  safeArea?: RectF
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

const DynamicNewTabTakeoverIframe =
  styled('iframe') <{ $dynamicContentHasLoaded: boolean }>`
  opacity: ${p => p.$dynamicContentHasLoaded ? 1 : 0};
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

export interface DynamicContentMessageCapabilities {
  onEventReported: (name: BraveAds.NewTabPageAdEventType) => void
  onAdEventReported?: (name: BraveAds.NewTabPageAdEventType) => void
  onOpenBraveSearch?: (query: string) => void
  onQueryAutocomplete?: (query: string) => void
  onMakeBraveSearchDefault?: () => void
}

// Reads a message posted from the dynamic-content background iframe and executes
// the appropriate capability. Exported for testing.
export function dispatchDynamicContentMessage(
  data: any,
  capabilities: DynamicContentMessageCapabilities,
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

function getDynamicContentOrigin(): string {
  return new URL(loadTimeData.getString('ntpNewTabTakeoverDynamicContentUrl')).origin
}

export function DynamicNewTabTakeover(props: Props) {
  const iframeRef = React.useRef<HTMLIFrameElement | null>(null)
  const { dynamicNewTabTakeoverInfo, safeArea } = props

  React.useEffect(() => {
    try {
      const dynamicContentOrigin = getDynamicContentOrigin()

      const listener = (event: MessageEvent) => {
        if (event.origin !== dynamicContentOrigin) {
          return
        }

        if (!iframeRef.current) {
          return
        }

        const { contentWindow } = iframeRef.current
        if (!event.source || event.source !== contentWindow || !event.data) {
          return
        }

        dispatchDynamicContentMessage(event.data, {
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
      console.error('Error setting up dynamic New Tab Takeover event listener')
      return () => { }
    }
  }, [props.onEventReported, props.onAdEventReported, props.onOpenBraveSearch, props.onQueryAutocomplete, props.onMakeBraveSearchDefault])

  React.useEffect(() => {
    if (!props.searchMatches || !iframeRef.current?.contentWindow) {
      return
    }
    try {
      iframeRef.current.contentWindow.postMessage({
        type: 'richMediaSearchMatches',
        value: props.searchMatches
      }, getDynamicContentOrigin())
    } catch (e) {
      console.error('Error posting search matches to dynamic New Tab Takeover iframe')
    }
  }, [props.searchMatches])

  React.useEffect(() => {
    if (!safeArea || !props.dynamicContentHasLoaded) {
      return
    }

    const contentWindow = iframeRef.current?.contentWindow
    if (!contentWindow) {
      return
    }

    try {
      contentWindow.postMessage(
        { type: 'richMediaSafeRect', value: safeArea },
        getDynamicContentOrigin())
    } catch (e) {
      console.error('Error posting dynamic New Tab Takeover safe area')
    }
  }, [safeArea, props.dynamicContentHasLoaded])

  return (
    <DynamicNewTabTakeoverIframe
      ref={iframeRef}
      $dynamicContentHasLoaded={props.dynamicContentHasLoaded}
      allow={iframeAllow}
      src={dynamicNewTabTakeoverInfo.url}
      sandbox='allow-scripts allow-same-origin'
      onLoad={props.onLoaded}>
    </DynamicNewTabTakeoverIframe>
  )
}
