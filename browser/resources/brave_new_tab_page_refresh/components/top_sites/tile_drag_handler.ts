/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

const autoScrollTimeout = 700

type ScrollDirection = 'forward' | 'back'

interface DragHandlerCallbacks {
  onScroll?: (direction: ScrollDirection) => void
  onDrop?: (dragFrom: number, dragTo: number) => void
}

interface Rect {
  x: number
  y: number
  height: number
  width: number
}

interface TileInfo {
  element: HTMLElement
  rect: Rect
}

interface DragInfo {
  container: HTMLElement
  target: HTMLElement
  tiles: TileInfo[]
  dragFrom: number
  dragTo: number
  rect: Rect
  scrollTimeout: any
  rtl: boolean
}

interface DragHandlerOptions {
  tileSelector: string
  autoScroll?: 'horizontal'
}

// Allows reordering of tiles within an optionally scrollable container using
// the Drag and Drop API.
export function createTileDragHandler(options: DragHandlerOptions) {
  let callbacks: DragHandlerCallbacks = {}
  let dragInfo: DragInfo | null = null

  function onDragStart(event: DragEvent) {
    const target = event.target as HTMLElement
    if (!target.hasAttribute('draggable') || !target.draggable) {
      return
    }

    if (!event.dataTransfer) {
      return
    }

    event.dataTransfer.effectAllowed = 'move'

    const container = event.currentTarget as HTMLElement
    const rect = container.getBoundingClientRect()
    const tileElements = [
      ...container.querySelectorAll<HTMLElement>(options.tileSelector),
    ]

    dragInfo = {
      container,
      target,
      tiles: tileElements.map((element) => ({
        element,
        rect: {
          x: element.offsetLeft,
          y: element.offsetTop,
          width: element.offsetWidth,
          height: element.offsetHeight,
        },
      })),
      dragFrom: -1,
      dragTo: -1,
      rect,
      scrollTimeout: 0,
      rtl: container.matches(':dir(rtl)'),
    }

    dragInfo.dragFrom = getTileIndex(event.clientX, event.clientY)

    target.classList.add('dragging')
    container.classList.add('sorting')
    document.addEventListener('dragover', onDragOver)
  }

  function onDragEnd() {
    finishDrag()
  }

  function onDragEnter(event: DragEvent) {
    if (!dragInfo) {
      return
    }
    const index = getTileIndex(event.clientX, event.clientY)
    if (index >= 0 && index !== dragInfo.dragFrom) {
      event.preventDefault()
    }
  }

  function onDragOver(event: DragEvent) {
    if (!dragInfo || !event.dataTransfer) {
      return
    }
    checkForScroll(event.clientX, event.clientY)
    const index = getTileIndex(event.clientX, event.clientY)
    if (index >= 0 && index !== dragInfo.dragFrom) {
      event.preventDefault()
    }
    if (index !== dragInfo.dragTo) {
      dragInfo.dragTo = index
      updateTilePositions()
    }
  }

  function onDrop(event: DragEvent) {
    if (!dragInfo) {
      return
    }
    event.preventDefault()
    const { dragFrom, dragTo } = dragInfo
    finishDrag()
    if (callbacks.onDrop) {
      callbacks.onDrop(dragFrom, dragTo)
    }
  }

  function checkForScroll(clientX: number, clientY: number) {
    if (!options.autoScroll || !dragInfo || clientX < 0 || clientY < 0) {
      return
    }

    let p = clientX
    let min = dragInfo.rect.x
    let max = min + dragInfo.rect.width

    let dir: ScrollDirection | '' = p < min ? 'back' : p > max ? 'forward' : ''
    if (options.autoScroll === 'horizontal' && dragInfo.rtl) {
      dir = dir === 'back' ? 'forward' : 'back'
    }

    if (!dir) {
      if (dragInfo.scrollTimeout) {
        clearTimeout(dragInfo.scrollTimeout)
        dragInfo.scrollTimeout = 0
      }
      return
    }

    if (dragInfo.scrollTimeout) {
      return
    }

    dragInfo.scrollTimeout = setTimeout(() => {
      if (!dragInfo) {
        return
      }
      dragInfo.scrollTimeout = 0
      if (callbacks.onScroll) {
        callbacks.onScroll(dir)
      }
    }, autoScrollTimeout)
  }

  function finishDrag() {
    if (!dragInfo) {
      return
    }
    dragInfo.dragFrom = -1
    dragInfo.dragTo = -1
    updateTilePositions()
    dragInfo.target.classList.remove('dragging')
    dragInfo.container.classList.remove('sorting')
    dragInfo = null
    document.removeEventListener('dragover', onDragOver)
  }

  function updateTilePositions() {
    if (!dragInfo) {
      return
    }
    const { tiles, dragFrom, dragTo } = dragInfo
    tiles.forEach((tile, i) => {
      let moveTo: TileInfo | null = null
      if (i > dragFrom && i <= dragTo) {
        moveTo = tiles[i - 1]
      } else if (i < dragFrom && i >= dragTo && dragTo >= 0) {
        moveTo = tiles[i + 1]
      }
      if (moveTo) {
        const dx = moveTo.rect.x - tile.rect.x
        const dy = moveTo.rect.y - tile.rect.y
        tile.element.style.translate = `${dx}px ${dy}px`
      } else {
        tile.element.style.translate = 'none'
      }
    })
  }

  function getTileIndex(clientX: number, clientY: number) {
    if (!dragInfo) {
      return -1
    }
    if (!inRect({ x: clientX, y: clientY }, dragInfo.rect)) {
      return -1
    }
    const point = getGridPoint(clientX, clientY)
    return dragInfo.tiles.findIndex((tile) => inRect(point, tile.rect))
  }

  function getGridPoint(clientX: number, clientY: number) {
    if (!dragInfo) {
      return { x: clientX, y: clientY }
    }
    return {
      x: clientX - dragInfo.rect.x + dragInfo.container.scrollLeft,
      y: clientY - dragInfo.rect.y,
    }
  }

  function observe(target: HTMLElement) {
    target.addEventListener('dragstart', onDragStart)
    target.addEventListener('dragend', onDragEnd)
    target.addEventListener('dragenter', onDragEnter)
    target.addEventListener('drop', onDrop)

    return () => {
      finishDrag()
      target.removeEventListener('dragstart', onDragStart)
      target.removeEventListener('dragend', onDragEnd)
      target.removeEventListener('dragenter', onDragEnter)
      target.removeEventListener('drop', onDrop)
    }
  }

  function setCallbacks(cb: DragHandlerCallbacks) {
    callbacks = cb
  }

  return { observe, setCallbacks }
}

function inRect(point: { x: number; y: number }, rect: Rect) {
  return (
    point.x >= rect.x
    && point.x < rect.x + rect.width
    && point.y >= rect.y
    && point.y < rect.y + rect.height
  )
}
