/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { TopSite } from '../../state/top_sites_state'
import { faviconURL } from '../../lib/favicon_url'

function sanitizeTileURL(url: string) {
  try {
    return new URL(url).toString()
  } catch {
    return ''
  }
}

interface Props {
  topSite: TopSite
  canDrag: boolean
  onContextMenu?: (event: React.MouseEvent) => void
}

export function TopSitesTile(props: Props) {
  const { favicon, title, url } = props.topSite

  function onContextMenu(event: React.MouseEvent) {
    if (props.onContextMenu) {
      event.preventDefault()
      props.onContextMenu(event)
    }
  }

  return (
    <a
      className='top-site-tile'
      href={sanitizeTileURL(url)}
      draggable={props.canDrag}
      onDragStart={(event) => {
        event.dataTransfer.setData('text/uri-list', url)
      }}
    >
      <span
        className='top-site-icon'
        onContextMenu={onContextMenu}
      >
        <img src={favicon || faviconURL(url)} />
      </span>
      <span className='top-site-title'>{title}</span>
    </a>
  )
}
