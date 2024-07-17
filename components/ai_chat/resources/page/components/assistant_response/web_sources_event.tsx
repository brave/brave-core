// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Url } from 'gen/url/mojom/url.mojom.m.js'
import getPageHandlerInstance, * as mojom  from '../../api/page_handler'
import styles from './web_sources_event.module.scss'

function WebSourceFaviconImage (props: { imageUrl: Url }) {
  const [dataUrl, setDataUrl] = React.useState<string>()

  React.useEffect(() => {
    let cancelled = false

    getPageHandlerInstance().pageHandler.getSearchFavicon(props.imageUrl)
    .then(({faviconImageData}) => {
      if (cancelled) return
      if (!faviconImageData) return

      const blob = new Blob([new Uint8Array(faviconImageData)], { type: 'image/*' })
      const url = URL.createObjectURL(blob)
      setDataUrl(url)
    })

    return () => {
      cancelled = true
    }
  }, [props.imageUrl])


  return (
    <img src={dataUrl} />
  )
}

export default function WebSourcesEvent (props: { sources: mojom.WebSource[] }) {
  const handleOpenSource = React.useCallback((e: React.MouseEvent, source: mojom.WebSource) => {
    e.preventDefault()
    getPageHandlerInstance().pageHandler.openURL(source.url)
  }, [])

  return (
    <div className={styles.sources}>
      <h4>Sources</h4>
      <ul>
        {props.sources.map(source => {
          const host = new URL(source.url.url).hostname
          return (
            <li>
              <a href={source.url.url} title={source.title} onClick={(e) => handleOpenSource(e, source)}>
                <WebSourceFaviconImage imageUrl={source.faviconUrl} />
                {host}
              </a>
            </li>
          )
          })}
      </ul>
    </div>
  )
}
