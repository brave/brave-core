// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { loadTimeData } from '$web-common/loadTimeData'
import * as React from 'react'
import styles from './rich_search_widget.module.scss'

const RICH_SEARCH_WIDGETS_ORIGIN = loadTimeData.getString(
  'richSearchWidgetsOrigin',
)
export default function RichSearchWidget(props: { jsonData: string }) {
  if (!RICH_SEARCH_WIDGETS_ORIGIN) {
    return null
  }

  const frameHandler = React.useCallback(
    (iframe: HTMLIFrameElement | null) => {
      if (iframe) {
        const sendContent = () => {
          iframe.contentWindow?.postMessage(
            JSON.parse(props.jsonData),
            RICH_SEARCH_WIDGETS_ORIGIN,
          )
        }

        iframe.onload = sendContent

        window.addEventListener('message', (event) => {
          // Ignore messages from other origins.
          if (event.origin !== RICH_SEARCH_WIDGETS_ORIGIN) {
            return
          }

          if (
            event.data.type !== 'resize'
            || event.source !== iframe.contentWindow
          ) {
            return
          }
          iframe.style.height = event.data.height + 'px'
        })
      }
    },
    [props.jsonData],
  )

  return (
    <iframe
      className={styles.richSearchWidget}
      sandbox='allow-scripts allow-same-origin allow-forms'
      src={`${RICH_SEARCH_WIDGETS_ORIGIN}/embed.html`}
      ref={frameHandler}
    />
  )
}
