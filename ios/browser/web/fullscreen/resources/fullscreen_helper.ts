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

if (
  !isFullscreenSupportedNatively
  && videosSupportFullscreen
  && !/mobile/i.test(navigator.userAgent)
) {
  HTMLElement.prototype.requestFullscreen = function (): Promise<void> {
    const element = this as FullscreenElement
    if (element.webkitRequestFullscreen !== undefined) {
      element.webkitRequestFullscreen()
      return Promise.resolve()
    }

    if (element.webkitEnterFullscreen !== undefined) {
      element.webkitEnterFullscreen()
      return Promise.resolve()
    }

    const video = element.querySelector<FullscreenVideoElement>('video')
    if (video?.webkitEnterFullscreen !== undefined) {
      video.webkitEnterFullscreen()
      return Promise.resolve()
    }

    return Promise.reject(new TypeError('Fullscreen request denied'))
  }

  const enabled = () => true
  Object.defineProperty(document, 'fullscreenEnabled', { get: enabled })
  Object.defineProperty(document.documentElement, 'fullscreenEnabled', {
    get: enabled,
  })
}
