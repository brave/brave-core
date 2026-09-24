// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  CrWebApi,
  gCrWeb,
} from '//ios/web/public/js_messaging/resources/gcrweb.js'

import {
  checkPageForMedia,
  clampDuration,
  getAllMediaElements,
  kTagPropertyName,
  notifyNode,
} from '//brave/ios/browser/playlist/resources/playlist_utils.js'

// Returns the current playback time of the media element previously tagged
// with `tag`, or 0 when no such element exists.
function getCurrentTimeForVideoWithTag(tag: string): number {
  for (const element of getAllMediaElements()) {
    if ((element as any)[kTagPropertyName] === tag) {
      return clampDuration(element.currentTime)
    }
  }
  return 0.0
}

// Determines whether `element` is visible within `view`. Used to ignore media
// hidden behind overlays or collapsed to zero size when resolving a long press.
function isElementVisible(view: Window, element: Element): boolean {
  const htmlElement = element as HTMLElement
  if (
    htmlElement.offsetWidth
    && htmlElement.offsetHeight
    && element.getClientRects().length
  ) {
    return true
  }

  const style = view.getComputedStyle(element)
  return (
    style.width !== '0px'
    && style.height !== '0px'
    && Number(style.opacity) > 0
    && style.display !== 'none'
    && style.visibility !== 'hidden'
  )
}

// Detects media at the (`x`, `y`) point that the user long pressed and reports
// it with `detected = false`, indicating an explicit user action rather than
// automatic detection. Both the top document and same-origin iframes are
// searched; cross-origin frames can't be inspected and are skipped.
function longPressedAtLocation(x: number, y: number): void {
  const detectMediaAtPoint = (
    view: Window,
    offsetX: number,
    offsetY: number,
  ): void => {
    const targets = view.document
      .elementsFromPoint(x - offsetX, y - offsetY)
      .filter(
        (element) =>
          element instanceof HTMLVideoElement
          || element instanceof HTMLAudioElement,
      )
      .filter((element) => isElementVisible(view, element))

    // When no media is directly under the point, fall back to any audio
    // element in the document (e.g. detached players with no visible node).
    if (targets.length === 0) {
      const audio = view.document.querySelector('audio')
      if (audio) {
        notifyNode(audio, '', false, false)
      }
      return
    }

    notifyNode(targets[0] as HTMLMediaElement, '', false, false)
  }

  // Media in the current document is at offset (0, 0) relative to the window.
  detectMediaAtPoint(window, 0, 0)

  // Media inside an iframe is at offset (0, 0) relative to the iframe's content
  // window, but the iframe itself is offset by its bounds relative to the
  // current window.
  try {
    for (const frame of document.querySelectorAll('iframe')) {
      const bounds = frame.getBoundingClientRect()
      if (frame.contentWindow) {
        detectMediaAtPoint(frame.contentWindow, bounds.left, bounds.top)
      }
    }
  } catch (error) {}
}

// Installs hooks that detect media attached to the page dynamically (after the
// initial document scan). This patches the shared HTMLMediaElement prototype
function installMediaDetection(): void {
  // Reserve a non-enumerable identifier slot on every media element so that
  // tags don't leak into property enumeration or serialization.
  Object.defineProperty(HTMLMediaElement.prototype, kTagPropertyName, {
    enumerable: false,
    configurable: false,
    writable: true,
    value: null,
  })

  const descriptor = Object.getOwnPropertyDescriptor(
    HTMLMediaElement.prototype,
    'src',
  )
  if (descriptor && descriptor.get && descriptor.set) {
    const getter = descriptor.get
    const setter = descriptor.set
    Object.defineProperty(HTMLMediaElement.prototype, 'src', {
      enumerable: descriptor.enumerable,
      configurable: descriptor.configurable,
      get(this: HTMLMediaElement) {
        return getter.call(this)
      },
      set(this: HTMLMediaElement, value: string) {
        setter.call(this, value)
        if (this instanceof HTMLVideoElement) {
          notifyNode(this, 'video', true, false)
        } else if (this instanceof HTMLAudioElement) {
          notifyNode(this, 'audio', true, false)
          // Re-scan shortly after an audio source is attached instead of
          // polling on an interval.
          setTimeout(() => checkPageForMedia(false), 100)
        }
      },
    })
  }

  const setVideoAttribute = HTMLVideoElement.prototype.setAttribute
  HTMLVideoElement.prototype.setAttribute = function (
    this: HTMLVideoElement,
    key: string,
    value: string,
  ): void {
    setVideoAttribute.call(this, key, value)
    if (key.toLowerCase() === 'src') {
      notifyNode(this, 'video', true, false)
    }
  }

  const setAudioAttribute = HTMLAudioElement.prototype.setAttribute
  HTMLAudioElement.prototype.setAttribute = function (
    this: HTMLAudioElement,
    key: string,
    value: string,
  ): void {
    setAudioAttribute.call(this, key, value)
    if (key.toLowerCase() === 'src') {
      notifyNode(this, 'audio', true, false)
      setTimeout(() => checkPageForMedia(false), 100)
    }
  }
}

const playlistApi = new CrWebApi('playlist')

// Gets the current time for a media element with the tag provided or 0 if that
// element does not exist.
playlistApi.addFunction(
  'getCurrentTimeForVideoWithTag',
  getCurrentTimeForVideoWithTag,
)

// Detects and reports media at a point the user long pressed. Pushed media is
// flagged with `detected = false` to distinguish it from automatic detection.
playlistApi.addFunction('longPressedAtLocation', longPressedAtLocation)

gCrWeb.registerApi(playlistApi)

installMediaDetection()
