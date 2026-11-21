// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// WebKit-prefixed fullscreen APIs that aren't part of the standard DOM types.
interface FullscreenDocument extends Document {
  webkitFullscreenEnabled?: boolean
  mozFullScreenEnabled?: boolean
  msFullscreenEnabled?: boolean
}

interface FullscreenElement extends HTMLElement {
  webkitRequestFullscreen?: () => void
  webkitEnterFullscreen?: () => void
}

interface FullscreenVideoElement extends HTMLVideoElement {
  webkitEnterFullscreen?: () => void
  webkitSupportsFullscreen?: boolean
  webkitSetPresentationMode?: (mode: string) => void
}

const doc = document as FullscreenDocument

const isFullscreenSupportedNatively =
  doc.fullscreenEnabled
  || doc.webkitFullscreenEnabled
  || doc.mozFullScreenEnabled
  || doc.msFullscreenEnabled

const videosSupportFullscreen =
  (HTMLVideoElement.prototype as FullscreenVideoElement).webkitEnterFullscreen
  !== undefined

function canEnterFullscreen(video: FullscreenVideoElement): boolean {
  return video.webkitSupportsFullscreen !== false
    && (video.webkitEnterFullscreen !== undefined
      || video.webkitSetPresentationMode !== undefined)
}

function findVideoInRoot(root: ParentNode): FullscreenVideoElement | null {
  const videos = root.querySelectorAll('video')
  for (const video of videos) {
    const candidate = video as FullscreenVideoElement
    if (canEnterFullscreen(candidate) && !candidate.paused) {
      return candidate
    }
  }
  for (const video of videos) {
    const candidate = video as FullscreenVideoElement
    if (canEnterFullscreen(candidate)) {
      return candidate
    }
  }
  for (const element of root.querySelectorAll('*')) {
    if (element.shadowRoot) {
      const nested = findVideoInRoot(element.shadowRoot)
      if (nested) {
        return nested
      }
    }
  }
  for (const iframe of root.querySelectorAll('iframe')) {
    try {
      const nestedDoc = iframe.contentDocument
      if (nestedDoc) {
        const nested = findVideoInRoot(nestedDoc)
        if (nested) {
          return nested
        }
      }
    } catch {
    }
  }
  return null
}

function findVideo(start: Element): FullscreenVideoElement | null {
  const element = start as FullscreenVideoElement
  if (element.localName === 'video' && canEnterFullscreen(element)) {
    return element
  }

  let current: Element | null = start
  while (current) {
    const found = findVideoInRoot(current)
    if (found) {
      return found
    }
    const root = current.getRootNode()
    if (root instanceof ShadowRoot) {
      current = root.host
      continue
    }
    current = current.parentElement
  }
  return findVideoInRoot(document)
}

function enterVideoFullscreen(video: FullscreenVideoElement): boolean {
  try {
    if (video.webkitSetPresentationMode) {
      video.webkitSetPresentationMode('fullscreen')
      return true
    }
    if (video.webkitEnterFullscreen) {
      video.webkitEnterFullscreen()
      return true
    }
  } catch {
  }
  return false
}

function requestVideoFullscreen(element: Element): boolean {
  const video = findVideo(element)
  return !!video && enterVideoFullscreen(video)
}

if (
  !isFullscreenSupportedNatively
  && videosSupportFullscreen
  && !/mobile/i.test(navigator.userAgent)
) {
  HTMLElement.prototype.requestFullscreen = function (): Promise<void> {
    if (requestVideoFullscreen(this)) {
      return Promise.resolve()
    }
    return Promise.reject(new TypeError('Fullscreen request denied'))
  }

  // Desktop players often call the prefixed API, which exists on iPhone but does not present the video when element fullscreen is disabled.
  ;(HTMLElement.prototype as FullscreenElement).webkitRequestFullscreen =
    function () {
      requestVideoFullscreen(this)
    }

  const enabled = () => true
  Object.defineProperty(document, 'fullscreenEnabled', { get: enabled })
  Object.defineProperty(document.documentElement, 'fullscreenEnabled', {
    get: enabled,
  })
}
