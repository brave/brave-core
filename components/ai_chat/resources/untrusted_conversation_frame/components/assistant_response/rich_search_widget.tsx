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

/** Gets the browser theme (which might be user configured) */
function getBrowserTheme() {
  const stringifyStyles = (stylesheet: CSSStyleSheet) => {
    return Array.from(stylesheet.cssRules || [])
      .filter((rule) => rule instanceof CSSStyleRule)
      .map((rule) => rule.cssText)
      .join('\n')
  }

  const nala = Array.from(document.styleSheets).find((s) =>
    s.href?.includes('nala.css'),
  )!

  const baseColors = Array.from(nala.cssRules || []).find(
    (rule) =>
      rule instanceof CSSImportRule && rule.href?.includes('theme/colors.css'),
  ) as CSSImportRule

  return stringifyStyles(baseColors.styleSheet!) + stringifyStyles(nala)
}

export default function RichSearchWidget(props: { jsonData: string }) {
  if (!RICH_SEARCH_WIDGETS_ORIGIN) {
    return null
  }

  const frameHandler = React.useCallback(
    (iframe: HTMLIFrameElement | null) => {
      if (iframe) {
        const sendContent = () => {
          iframe.contentWindow?.postMessage(
            { type: 'theme', styles: getBrowserTheme() },
            RICH_SEARCH_WIDGETS_ORIGIN,
          )
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
      sandbox='allow-scripts allow-same-origin allow-forms allow-popups'
      src={`${RICH_SEARCH_WIDGETS_ORIGIN}/embed.html`}
      ref={frameHandler}
    />
  )
}
