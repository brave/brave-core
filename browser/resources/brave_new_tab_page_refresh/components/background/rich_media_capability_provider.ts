/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useBackgroundActions } from '../../context/background_context'
import { braveSearchHost } from '../../state/search_state'
import {
  useSearchMatches,
  useSearchActions,
} from '../../context/search_context'
import { openLink } from '../common/link'
import { useWidgetLayoutReady } from '../app_layout_ready'
import { debounce } from '$web-common/debounce'

import {
  RichMediaFrameHandle,
  handleIncomingMessage,
} from './rich_media_capabilities'

interface MessageHandlerOptions {
  destinationUrl?: string
}

// Returns a callback that will handle messages received from the rich media
// background iframe.
export function useRichMediaMessageHandler(
  frameHandle: RichMediaFrameHandle | undefined,
  options: MessageHandlerOptions,
) {
  const { destinationUrl } = options
  const actions = useBackgroundActions()
  const queryAutocomplete = useBraveSearchAutocomplete(frameHandle)

  return React.useCallback(
    (data: unknown) => {
      handleIncomingMessage(data, {
        notifyAdEvent(eventType) {
          actions.notifySponsoredRichMediaEvent(eventType)
        },
        openDestinationUrl() {
          if (destinationUrl) {
            openLink(destinationUrl)
          }
        },
        openBraveSearch(pathAndQuery) {
          openLink(`https://${braveSearchHost}/${pathAndQuery}`)
        },
        queryBraveSearchAutocomplete(query) {
          queryAutocomplete(query)
        },
      })
    },
    [destinationUrl, frameHandle],
  )
}

// Posts a message to the rich media background iframe containing the current
// list of search matches.
function useBraveSearchAutocomplete(frameHandle?: RichMediaFrameHandle) {
  const inputKey = 'rich-media-search-box'
  const searchActions = useSearchActions()
  const searchMatches = useSearchMatches(inputKey)

  React.useEffect(() => {
    if (!frameHandle || !searchMatches) {
      return
    }

    const postSearchMatches = debounce(() => {
      frameHandle.postMessage({
        type: 'richMediaSearchMatches',
        value: searchMatches,
      })
    }, 120)

    postSearchMatches()
  }, [frameHandle, searchMatches])

  return (query: string) => {
    searchActions.setActiveSearchInputKey(inputKey)
    searchActions.queryAutocomplete(query, braveSearchHost)
  }
}

// Posts a message to the rich media background iframe containing a rectangle
// that is empty of content and can be used to display interactive elements.
export function useSafeAreaReporter(frameHandle?: RichMediaFrameHandle) {
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
          height: rect.height,
        },
      })
    }, 120)

    postSafeArea()

    const resizeObserver = new ResizeObserver(postSafeArea)
    resizeObserver.observe(safeArea)
    return () => {
      resizeObserver.disconnect()
    }
  }, [widgetLayoutReady, frameHandle])
}
