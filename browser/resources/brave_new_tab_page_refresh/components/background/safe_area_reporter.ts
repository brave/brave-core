/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { RichMediaFrameHandle } from './rich_media_capabilities'
import { useWidgetLayoutReady } from '../app_layout_ready'
import { debounce } from '$web-common/debounce'

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
