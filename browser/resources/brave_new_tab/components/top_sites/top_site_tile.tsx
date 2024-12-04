/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { TopSite } from '../../models/top_sites_model'
import { faviconURL } from '../../lib/favicon_url'

function sanitizeTileURL(url: string) {
  try {
    return new URL(url).toString()
  } catch {
    return ''
  }
}

export type DropLocation = 'before' | 'after'

function getDropLocation(x: number, rect: DOMRect): DropLocation {
  const mid = rect.x + (rect.width / 2)
  return x < mid ? 'before' : 'after'
}

function relativeOffset(element: HTMLElement, offsetParent: Element | null) {
  const offset = { top: element.offsetTop, left: element.offsetLeft }
  let parent = element.offsetParent as HTMLElement
  while (parent && parent !== offsetParent) {
    offset.top += parent.offsetTop
    offset.left += parent.offsetLeft
    parent = parent.offsetParent as HTMLElement
  }
  return offset
}

function updateDropIndicator(tile: HTMLElement, location: DropLocation | null) {
  const indicator = document.querySelector<HTMLElement>('.tile-drop-indicator')
  if (!indicator) {
    return
  }
  if (location) {
    indicator.classList.add('dragging')
    const offset = relativeOffset(tile, indicator.offsetParent)
    if (location === 'after') {
      offset.left += tile.offsetWidth
    }
    indicator.style.top = offset.top + 'px'
    indicator.style.left = offset.left + 'px'
  } else {
    indicator.classList.remove('dragging')
  }
}

interface Props {
  topSite: TopSite
  canDrag: boolean
  onRightClick: (event: React.MouseEvent) => void
  onDrop: (url: string, location: DropLocation) => void
}

export function TopSitesTile(props: Props) {
  const rootRef = React.useRef<HTMLAnchorElement>(null)
  const dragInfo = React.useRef({ enterCount: 0, updateFrame: 0 })

  const updateIndicator = React.useCallback(
    (point: { x: number, y: number } | null) => {
      if (dragInfo.current.updateFrame) {
        cancelAnimationFrame(dragInfo.current.updateFrame)
      }
      dragInfo.current.updateFrame = requestAnimationFrame(() => {
        dragInfo.current.updateFrame = 0
        const elem = rootRef.current
        if (!elem) {
          return
        }
        if (point) {
          const rect = elem.getBoundingClientRect()
          const location = getDropLocation(point.x, rect)
          updateDropIndicator(elem, location)
        } else {
          updateDropIndicator(elem, null)
        }
      })
    }, [])

  const { favicon, title, url } = props.topSite

  function onContextMenu(event: React.MouseEvent) {
    event.preventDefault()
  }

  function onMouseUp(event: React.MouseEvent) {
    if (event.button === 2) {
      props.onRightClick(event)
    }
  }

  function onDragStart(event: React.DragEvent<HTMLElement>) {
    event.dataTransfer.setData('text/uri-list', url)
    event.dataTransfer.setData('text/top-site-url', url)
  }

  function isTopSiteDrag(event: React.DragEvent) {
    return event.dataTransfer.types.includes('text/top-site-url')
  }

  function onDragEnter(event: React.DragEvent<HTMLElement>) {
    dragInfo.current.enterCount += 1;
    if (dragInfo.current.enterCount === 1 && isTopSiteDrag(event)) {
      event.preventDefault()
    }
  }

  function onDragOver(event: React.DragEvent<HTMLElement>) {
    if (dragInfo.current.enterCount > 0 && isTopSiteDrag(event)) {
      event.preventDefault()
      event.dataTransfer.dropEffect = 'move'
      if (rootRef.current) {
        updateIndicator({ x: event.clientX, y: event.clientY })
      }
    }
  }

  function onDragLeave(event: React.DragEvent<HTMLElement>) {
    dragInfo.current.enterCount -= 1;
    if (dragInfo.current.enterCount <= 0) {
      updateIndicator(null)
    }
  }

  function onDrop(event: React.DragEvent) {
    event.preventDefault()

    dragInfo.current.enterCount = 0
    updateIndicator(null)

    const dragURL = event.dataTransfer.getData('text/top-site-url')
    if (dragURL && rootRef.current) {
      const rect = rootRef.current.getBoundingClientRect()
      let location = getDropLocation(event.clientX, rect)
      if (rootRef.current.matches(':dir(rtl)')) {
        // Notify the parent of the drop's logical, rather than physical
        // location. In RTL, this means we need to reverse the direction.
        location = location === 'before' ? 'after' : 'before'
      }
      props.onDrop(dragURL, location)
    }
  }

  return (
    <a
      ref={rootRef}
      className='top-site-tile'
      href={sanitizeTileURL(url)}
      onDragEnter={onDragEnter}
      onDragOver={onDragOver}
      onDragLeave={onDragLeave}
      onDrop={onDrop}
    >
      <span
        className='top-site-icon'
        onContextMenu={onContextMenu}
        onMouseUp={onMouseUp}
        draggable={props.canDrag ? 'true' : 'false'}
        onDragStart={onDragStart}
      >
        <img src={favicon || faviconURL(url)} />
      </span>
      <span className='top-site-title'>
        {title}
      </span>
    </a>
  )
}
